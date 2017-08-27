#pragma once
#include <thread>
#include <map>
#include "std/extension.hpp"
#include "net/acceptor.hpp"
#include "net/syslogstream.hpp"
#include "ws/frame.hpp"
#include "http/base64.hpp"

namespace ws {

using namespace std::string_literals;
using namespace std::chrono_literals;

using namespace http;

class server
{
public:

    void listen(const std::string& serice_or_port = "ws"s)
    try
    {
        auto acceptor = net::acceptor{"localhost"s, serice_or_port};
        acceptor.timeout(1h);
        net::slog << net::info << "Accepting connections at localhost." << serice_or_port << net::flush;
        while(true)
        {
            auto client = acceptor.accept();
            auto worker = std::thread{[&](){handle(std::move(client));}};
            worker.detach();
            std::this_thread::sleep_for(3ms);
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

    void handle(net::endpointstream client)
    {
        using namespace std;

        // while(client)
        {
            auto method = ""s, uri = ""s, version = ""s;
            auto headers = std::map<std::string,std::string>{};

            client >> method >> uri >> version;
            net::slog << net::info << method << ' ' << uri << ' ' << version << net::flush;

            client >> std::ws;

            while(client && client.peek() != '\r')
            {
                auto name = ""s, value = ""s;
                getline(client, name, ':');
                ext::trim(name);
                getline(client, value);
                ext::trim(value);
                headers[name] = value;
                net::slog << net::info << name << ": " << value << net::flush;
            }
            client.ignore(2); // crlf

            if(uri == "/favicon.ico")
            {
                client << "HTTP/1.1 404 Not Found"                                 << net::crlf
                       << "Date: " << ext::to_rfc1123(chrono::system_clock::now()) << net::crlf
                       << "Server: net4cpp/1.1"                                    << net::crlf
                       << "Content-Type: text/html"                                << net::crlf
                       << "Content-Length: 0"                                      << net::crlf
                       << net::crlf << net::flush;
            }
            else if(m_authenticate &&
                   (headers.count("Authorization") == 0 || headers["Authorization"] == "Basic Og=="))
            {
                client << "HTTP/1.1 401 Unauthorized status"                       << net::crlf
                       << "Date: " << ext::to_rfc1123(chrono::system_clock::now()) << net::crlf
                       << "Server: net4cpp/1.1"                                    << net::crlf
                       << "WWW-Authenticate: Basic realm=\"User Visible Realm\""   << net::crlf
                       << "Content-Type: text/html"                                << net::crlf
                       << "Content-Length: 0"                                      << net::crlf
                       << net::crlf << net::flush;
            }
            else if(headers["Upgrade"] == "websocket")
            {
                net::slog << net::debug << "Upgrading to WebSocket" << net::flush;

                const auto guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

                auto client_key = headers["Sec-WebSocket-Key"];

                auto server_key = base64::encode(client_key+guid);

                client << "HTTP/1.1 101 Switching Protocols"                       << net::crlf
                    //    << "Date: " << ext::to_rfc1123(chrono::system_clock::now()) << net::crlf
                    //    << "Server: net4cpp/1.1"                                    << net::crlf

                       << "Upgrade: websocket"                                     << net::crlf
                       << "Connection: Upgrade"                                    << net::crlf
                       << "Sec-WebSocket-Accept: " << server_key                   << net::crlf
                       << "Sec-WebSocket-Version: 13"                              << net::crlf

                    //    << "Access-Control-Allow-Origin: *"                         << net::crlf
                    //    << "Access-Control-Allow-Methods: GET"                      << net::crlf
                    //    << "Access-Control-Allow-Credentials: true"                 << net::crlf
                       << net::crlf << net::flush;

                auto frame = ws::frame{};
                client >> frame;
                if(client)
                    net::slog << net::debug << "Received ws-frame opcode " << std::to_integer<int>(frame.header.opcode) << net::flush;
                else
                    net::slog << net::debug << "Client closed Connection" << net::flush;
            }
        }
    }

    bool m_authenticate = false;
};

} // namespace ws
