#pragma once
#include <map>
#include <set>
#include <tuple>
#include <regex>
#include <string>
#include <thread>
#include "net/acceptor.hpp"
#include "net/syslogstream.hpp"
#include "http/headers.hpp"
#include "http/ws/frame.hpp"
#include "cryptic/sha1.hpp"

namespace http {

using namespace std::literals;

class controller
{
public:

    using callback = std::function<std::string(std::string_view,std::string_view,const http::headers&)>;

    void text(const std::string& content)
    {
        m_content_type = "text/plain"s;
        m_callback = [&](std::string_view,std::string_view,const http::headers&){return content;};
    }

    void html(const std::string& content)
    {
        m_content_type = "text/html"s;
        m_callback = [&](std::string_view,std::string_view,const http::headers&){return content;};
    }

    void css(const std::string& content)
    {
        m_content_type = "text/css"s;
        m_callback = [&](std::string_view,std::string_view,const http::headers&){return content;};
    }

    void script(const std::string& content)
    {
        m_content_type = "application/javascript"s;
        m_callback = [&](std::string_view,std::string_view,const http::headers&){return content;};
    }

    void json(const std::string& content)
    {
        m_content_type = "application/json"s;
        m_callback = [&](std::string_view,std::string_view,const http::headers&){return content;};
    }

    void xml(const std::string& content)
    {
        m_content_type = "application/xml"s;
        m_callback = [&](std::string_view,std::string_view,const http::headers&){return content;};
    }

    void response(std::string_view content_type, callback cb)
    {
        m_content_type = content_type;
        m_callback = cb;
    }

    std::tuple<std::string,std::string> render(std::string_view request, std::string_view body, const http::headers& headers) const
    {
        return {m_callback(request,body,headers),m_content_type};
    }

private:

    std::string m_content_type = "*/*"s;

    callback m_callback = [](std::string_view, std::string_view, const http::headers&){return "Not Found"s;};
};

class server
{
public:

    controller& get(const std::string& path)
    {
        m_methods.insert("GET"s);
        return m_router[path]["GET"s];
    }

    controller& head(const std::string& path)
    {
        m_methods.insert("HEAD"s);
        return m_router[path]["HEAD"s];
    }

    controller& post(const std::string& path)
    {
        m_methods.insert("POST"s);
        return m_router[path]["POST"s];
    }

    controller& put(const std::string& path)
    {
        m_methods.insert("PUT"s);
        return m_router[path]["PUT"s];
    }

    controller& patch(const std::string& path)
    {
        m_methods.insert("PATCH"s);
        return m_router[path]["PATCH"s];
    }

    controller& destroy(const std::string& path)
    {
        m_methods.insert("DELETE"s);
        return m_router[path]["DELETE"s];
    }

    controller& ws(const std::string& path)
    {
        m_methods.insert("GET"s);
        return m_router[path]["WS"s];
    }

    void listen(std::string_view service_or_port = "http")
    {
        slog << notice("http") << "starting up at "s << service_or_port << flush;
        auto endpoint = acceptor{"0.0.0.0", service_or_port};
        endpoint.timeout(0s); // no timeout
        slog << notice("http") << "started up at " << endpoint.host() << ":" << endpoint.service_or_port() << flush;
        while(true)
        {
            slog << info("http") << "accepting connections" << flush;
            auto client = endpoint.accept();
            std::thread{[client = std::move(client), this]() mutable {handle(client);}}.detach();
        }
    }

    void public_paths(std::string_view regex)
    {
        m_public_paths = regex;
    }

    void credentials(std::initializer_list<std::string> tokens)
    {
        m_credentials.insert(tokens);
    }

private:

    auto date() const
    {
        return ext::to_rfc1123(std::chrono::system_clock::now());
    }

    const auto& host() const
    {
        static auto s_host = syslog::gethostname();
        return s_host;
    }

    auto methods() const
    {
        auto tmp = ""s;
        for(const auto&  m : m_methods)
        {
            if(not tmp.empty()) tmp += ", "s;
            tmp += m;
        }
        return tmp;
    }

    const auto& content_type() const
    {
        return m_content_type;
    }

    void handle(auto& client)
    {
        auto& [stream,endpoint,port] = client;
        slog << notice("http") << "accepted connection from " << endpoint << ":" << port << flush;

        try
        {
            while(stream)
            {
                auto method = ""s, uri = ""s, version = ""s;
                stream >> method >> uri >> version >> crlf;

                if(not stream.good()) break;

                slog << notice("http") << "request \"" << method << ' ' << uri << ' ' << version << "\"" << flush;

                auto headers = http::headers{};
                stream >> headers >> crlf;

                auto origin = "*"s;
                if(headers.contains("origin"))
                    origin = headers["origin"];

                auto authorization = ""s;
                if(headers.contains("authorization"))
                    authorization = headers["authorization"];

                auto content_length = 0ull;
                if(headers.contains("content-length"))
                    content_length = std::stoll(headers["content-length"]);

                auto upgrade = ""s;
                if(headers.contains("upgrade"))
                    upgrade = headers["upgrade"];

                auto body = std::string(content_length,' ');
                for(auto& c : body)
                    c = stream.get();

                slog << debug("http") << "request body \"" << body << "\"" << flush;

                if(version != "HTTP/1.1")
                {
                    stream << "HTTP/1.1 505 HTTP Version Not Supported" << crlf
                           << "Date: "         << date()                << crlf
                           << "Server: "       << host()                << crlf
                           << "Content-Type: " << content_type()        << crlf
                           << "Content-Length: 0"                       << crlf
                           << crlf << flush;
                }
                else if(not m_methods.contains(method))
                {
                    stream << "HTTP/1.1 400 Bad Request"                    << crlf
                           << "Date: "         << date()                    << crlf
                           << "Server: "       << host()                    << crlf
                           << "Content-Type: " << content_type()            << crlf
                           << "Content-Length: 0"                           << crlf
                           << "Access-Control-Allow-Methods: " << methods() << crlf
                           << crlf << flush;
                }
                else if(uri == "/favicon.ico")
                {
                    stream << "HTTP/1.1 404 Not Found"           << crlf
                           << "Date: "         << date()         << crlf
                           << "Server: "       << host()         << crlf
                           << "Content-Type: " << content_type() << crlf
                           << "Content-Length: 0"                << crlf
                           << crlf << flush;
                }
                else if(not std::regex_match(uri,std::regex(m_public_paths)) and not m_credentials.contains(authorization))
                {
                    stream << "HTTP/1.1 401 Unauthorized status"                     << crlf
                           << "Date: "         << date()                             << crlf
                           << "Server: "       << host()                             << crlf
                           << "Content-Type: " << content_type()                     << crlf
                           << "Content-Length: 0"                                    << crlf
                           << "WWW-Authenticate: Basic realm=\"User Visible Realm\"" << crlf
                           << crlf << flush;
                }
                else if (upgrade == "websocket")
                {
                    slog << debug << "Upgrading to WebSocket" << flush;

                    constexpr auto guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
                    const auto client_key = headers["sec-websocket-key"];
                    const auto server_key = client_key+guid;
                    const auto server_key_sha1_base64 = cryptic::sha1::base64(server_key);

                    slog << debug << "Sec-WebSocket-Key: " << client_key << flush;
                    slog << debug << "Concatenated: " << server_key << flush;
                    slog << debug << "Sec-WebSocket-Accept: " << server_key_sha1_base64 << flush;

                    stream << "HTTP/1.1 101 Switching Protocols"                  << crlf
                           << "Upgrade: websocket"                                << crlf
                           << "Connection: Upgrade"                               << crlf
                           << "Sec-WebSocket-Accept: " << server_key_sha1_base64  << crlf
                           << crlf << flush;

                    while(stream)
                    {
                        if(!stream.wait_for(10s))
                        {
                            const auto ping = ws::frame{ws::ping,0b0,0b0,0b0,0b1,0b0,0b0000000,0b0};
                            stream << ping << flush;
                            slog << debug << "Sent ping ws-frame:" << std::bitset<16>{ping.bits} << flush;
                        }
                        else
                        {
                            auto frame = ws::frame{};
                            stream >> frame;

                            slog << debug << "Received ws-frame:" << std::bitset<16>{frame.bits} << flush;
                            slog << debug << "fin:" << std::bitset<1>{frame.header.fin} << flush;
                            slog << debug << "opcode: " << std::bitset<4>{frame.header.opcode} << flush;
                            slog << debug << "masked: " << std::bitset<1>{frame.header.masked} << flush;
                            slog << debug << "payload-length: " << std::dec << (int) frame.header.payload_length << flush;

                            if(frame.header.masked)
                                slog << debug << "masking-key " << std::hex << frame.masking_key << flush;

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
                                stream << flush;
                                slog << debug << "Sent close ws-frame: " << std::bitset<16>{close.bits} << flush;
                                break;
                            }
                            else if(frame.header.opcode != ws::pong)
                            {
                                auto content = ""s,  type = ""s;
                                for(auto& [path,controller] : m_router)
                                    if(std::regex_match(uri,std::regex(path)))
                                        std::tie(content,type) = controller["WS"].render(uri,body,headers);

                                auto reply = ws::frame{ws::text,0b0,0b0,0b0,0b1,0b0,0b0000000,0b0};
                                reply.header.payload_length = content.length();
                                stream << reply;
                                for (const auto c : content)
                                    stream << c;
                                stream << flush;
                                slog << debug << "Sent text ws-frame: " << std::bitset<16>{reply.bits} << flush;
                                for (const auto c : content)
                                    std::clog << c;
                                std::clog << std::endl;
                                continue;
                            }
                        }
                    }
                }
                else // Normal HTTP process
                {
                    auto content = ""s,  type = ""s;
                    for(auto& [path,controller] : m_router)
                        if(std::regex_match(uri,std::regex(path)))
                            std::tie(content,type) = controller[method].render(uri,body,headers);

                    if(not type.empty())
                        stream << "HTTP/1.1 200 OK"                                           << crlf
                               << "Date: " << date()                                          << crlf
                               << "Server: " << host()                                        << crlf
                               << "Content-Type: " << type                                    << crlf
                               << "Content-Length: " << content.length()                      << crlf
                               << "Access-Control-Allow-Methods: " << methods()               << crlf
                               << "Cache-Control: private"                                    << crlf
                               << "Access-Control-Allow-Origin: " << origin                   << crlf
                               << "Access-Control-Allow-Headers: Content-Type, Authorization" << crlf
                               << "Access-Control-Allow-Credentials: true"                    << crlf
                               << crlf
                               << (method != "HEAD"s ? content : ""s) << flush;
                    else
                        stream << "HTTP/1.0 404 Not Found"                                    << crlf
                               << "Date: " << date()                                          << crlf
                               << "Server: " << host()                                        << crlf
                               << "Content-Type: " << content_type()                          << crlf
                               << "Content-Length: 0"                                         << crlf
                               << "Access-Control-Allow-Methods: " << methods()               << crlf
                               << "Cache-Control: private"                                    << crlf
                               << "Access-Control-Allow-Origin: " << origin                   << crlf
                               << "Access-Control-Allow-Headers: Content-Type, Authorization" << crlf
                               << "Access-Control-Allow-Credentials: true"                    << crlf
                               << crlf << flush;
                }
            }
            slog << info("http") << "connection closed" << flush;
        }
        catch(const std::exception& e)
        {
            stream << "HTTP/1.0 500 Internal Server Error" << crlf
                   << "Date: " << date()                   << crlf
                   << "Server: " << host()                 << crlf
                   << "Content-Type: " << content_type()   << crlf
                   << "Content-Length: 0"                  << crlf
                   << crlf << flush;

            slog << error("http") << "error: " << std::quoted(e.what()) << flush;
        }
    }

    using router = std::map<std::string,std::map<std::string,controller>, std::less<>>;

    router m_router = {};

    std::string m_content_type = "*/*"s;

    std::set<std::string> m_methods = {"HEAD"s, "OPTIONS"s};

    std::string m_public_paths = ".*";

    std::set<std::string> m_credentials = {};
};

} // namespace http
