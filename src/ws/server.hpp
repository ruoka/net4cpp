#pragma once
#include <map>
#include <bitset>
#include <thread>
#include "std/extension.hpp"
#include "net/acceptor.hpp"
#include "net/syslogstream.hpp"
#include "cryptic/sha1.hpp"
#include "ws/frame.hpp"

namespace ws {

using namespace std;
using namespace std::string_literals;
using namespace std::chrono_literals;

class server
{
public:

    void listen(const std::string& serice_or_port = "ws"s)
    try
    {
        auto endpoint = net::acceptor{"localhost"s, serice_or_port};
        endpoint.timeout(1h);
        net::slog << net::info("http") << "Accepting connections at localhost." << serice_or_port << net::flush;
        while(true)
        {
            net::slog << net::info("http") << "accepting connections" << net::flush;
            auto client = endpoint.accept();
            std::thread{[client = std::move(client), this]() mutable {handle(client);}}.detach();
        }
    }
    catch(const std::exception& e)
    {
        net::slog << net::error << e.what() << net::flush;
        throw;
    }

    bool authenticate() const
    {
        return m_authenticate;
    }

    void authenticate(bool b)
    {
        m_authenticate = b;
    }

private:

    using sha1 = cryptic::sha1;

    void handle(auto& client)
    {
        auto& [stream,endpoint,port] = client;
        net::slog << net::notice("http") << "accepted connection from " << endpoint << ":" << port << net::flush;

        auto method = ""s, uri = ""s, version = ""s;
        auto headers = std::map<std::string,std::string>{};

        stream >> method >> uri >> version;
        net::slog << net::info << method << ' ' << uri << ' ' << version << net::flush;

        stream >> std::ws;

        while(stream && stream.peek() != '\r')
        {
            auto name = ""s, value = ""s;
            getline(stream, name, ':');
            ext::trim(name);
            getline(stream, value);
            ext::trim(value);
            headers[name] = value;
            net::slog << net::info << name << ": " << value << net::flush;
        }
        stream.ignore(2); // crlf

        if(headers["Upgrade"] != "websocket")
        {
            stream << "HTTP/1.1 404 Not Found"                                 << net::crlf
                   << "Date: " << ext::to_rfc1123(chrono::system_clock::now()) << net::crlf
                   << "Server: net4cpp/1.1"                                    << net::crlf
                   << "Content-Type: text/html"                                << net::crlf
                   << "Content-Length: 0"                                      << net::crlf
                   << net::crlf << net::flush;
        }
        else if(m_authenticate &&
          (headers.count("Authorization") == 0 || headers["Authorization"] == "Basic Og=="))
        {
            stream << "HTTP/1.1 401 Unauthorized status"                       << net::crlf
                   << "Date: " << ext::to_rfc1123(chrono::system_clock::now()) << net::crlf
                   << "Server: net4cpp/1.1"                                    << net::crlf
                   << "WWW-Authenticate: Basic realm=\"User Visible Realm\""   << net::crlf
                   << "Content-Type: text/html"                                << net::crlf
                   << "Content-Length: 0"                                      << net::crlf
                   << net::crlf << net::flush;
        }
        else
        {
            net::slog << net::debug << "Upgrading to WebSocket" << net::flush;

            constexpr auto guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            const auto client_key = headers["Sec-WebSocket-Key"];
            const auto server_key = client_key+guid;
            const auto server_key_sha1_base64 = sha1::base64(server_key);

            net::slog << net::debug << "Sec-WebSocket-Key: " << client_key << net::flush;
            net::slog << net::debug << "Concatenated: " << server_key << net::flush;
            net::slog << net::debug << "Sec-WebSocket-Accept: " << server_key_sha1_base64 << net::flush;

            stream << "HTTP/1.1 101 Switching Protocols"                        << net::crlf
//                   << "Date: " << ext::to_rfc1123(chrono::system_clock::now())  << net::crlf
//                   << "Server: net4cpp/1.1"                                     << net::crlf
                   << "Upgrade: websocket"                                      << net::crlf
                   << "Connection: Upgrade"                                     << net::crlf
                   << "Sec-WebSocket-Accept: " << server_key_sha1_base64 << " " << net::crlf
//                   << "Sec-WebSocket-Version: 13"                               << net::crlf
//                   << "Sec-WebSocket-Protocol: echo"                            << net::crlf
                   << net::crlf << net::flush;

            stream.wait_for(1s);

            while(stream)
            {
                if(!stream.wait_for(10s))
                {
                    const auto ping = ws::frame{ws::ping,0b0,0b0,0b0,0b1,0b0,0b0000000,0b0};
                    stream << ping << net::flush;
                    net::slog << net::debug << "Sent ping ws-frame:" << std::bitset<16>{ping.bits} << net::flush;
                }
                else
                {
                    auto frame = ws::frame{};
                    stream >> frame;

                    net::slog << net::debug << "Received ws-frame:" << std::bitset<16>{frame.bits} << net::flush;
                    net::slog << net::debug << "fin:" << std::bitset<1>{frame.header.fin} << net::flush;
                    net::slog << net::debug << "opcode: " << std::bitset<4>{frame.header.opcode} << net::flush;
                    net::slog << net::debug << "masked: " << std::bitset<1>{frame.header.masked} << net::flush;
                    net::slog << net::debug << "payload-length: " << std::dec << (int) frame.header.payload_length << net::flush;

                    if(frame.header.masked)
                        net::slog << net::debug << "masking-key " << std::hex << frame.masking_key << net::flush;

                    for (const auto c : frame.payload_data)
                        std::clog << c;
                    std::clog << std::endl;

                    if(frame.header.opcode == ws::close)
                    {
                        auto close = ws::frame{ws::close,0b0,0b0,0b0,0b1,0b0,0b0000000,0b0};
                        close.header.payload_length = frame.header.payload_length;
                        stream << close;
                        for (const auto c : frame.payload_data)
                            stream << c;
                        stream << net::flush;
                        net::slog << net::debug << "Sent close ws-frame: " << std::bitset<16>{close.bits} << net::flush;
                        break;
                    }
                    else if(frame.header.opcode != ws::pong)
                    {
                        auto echo = ws::frame{ws::text,0b0,0b0,0b0,0b1,0b0,0b0000000,0b0};
                        echo.header.payload_length = frame.header.payload_length;
                        stream << echo;
                        for (const auto c : frame.payload_data)
                            stream << c;
                        stream << net::flush;
                        net::slog << net::debug << "Sent text ws-frame: " << std::bitset<16>{echo.bits} << net::flush;
                        for (const auto c : frame.payload_data)
                            std::clog << c;
                        std::clog << std::endl;
                        continue;
                    }
                }
            }
            net::slog << net::info << "Client closed Connection" << net::flush;
        }
    }

    bool m_authenticate = false;
};

} // namespace ws
