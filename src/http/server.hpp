#pragma once
#include <set>
#include <tuple>
#include <regex>
#include <string>
#include <thread>
#include <functional>
#include <unordered_map>
#include "net/acceptor.hpp"
#include "http/headers.hpp"

namespace http {

using namespace std::chrono_literals;

class controller
{
public:

    using callback = std::function<std::string(const std::string&,const std::string&)>;

    void text(const std::string& content)
    {
        m_content_type = "text/plain"s;
        m_callback = [&](const std::string&,const std::string&){return content;};
    }

    void html(const std::string& content)
    {
        m_content_type = "text/html"s;
        m_callback = [&](const std::string&,const std::string&){return content;};
    }

    void css(const std::string& content)
    {
        m_content_type = "text/css"s;
        m_callback = [&](const std::string&,const std::string&){return content;};
    }

    void script(const std::string& content)
    {
        m_content_type = "application/javascript"s;
        m_callback = [&](const std::string&,const std::string&){return content;};
    }

    void json(const std::string& content)
    {
        m_content_type = "application/json"s;
        m_callback = [&](const std::string&,const std::string&){return content;};
    }

    void xml(const std::string& content)
    {
        m_content_type = "application/xml"s;
        m_callback = [&](const std::string&,const std::string&){return content;};
    }

    void response(const std::string& content_type, callback cb)
    {
        m_content_type = content_type;
        m_callback = cb;
    }

    std::tuple<std::string,std::string> render(const std::string& request = ""s, const std::string& body = ""s) const
    {
        return {m_callback(request,body),m_content_type};
    }

    void content_type(const std::string& type)
    {
        m_content_type = type;
    }

    const std::string& content_type() const
    {
        return m_content_type;
    }

private:

    std::string m_content_type = "*/*"s;

    callback m_callback = [](const std::string&,const std::string&){return "Not Found"s;};
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

    void listen(const std::string& serice_or_port = "http"s)
    {
        auto acceptor = net::acceptor{"localhost"s, serice_or_port};
        acceptor.timeout(1h);
        while(true)
        {
            auto client = acceptor.accept();
            auto worker = std::thread{[&](){handle(std::move(client));}};
            worker.detach();
            std::this_thread::sleep_for(3ms);
        }
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

    auto date() const
    {
        return ext::to_rfc1123(std::chrono::system_clock::now());
    }

    const auto& host() const
    {
        return m_host;
    }

    auto methods() const
    {
        return "OPTIONS, HEAD, GET, POST, PUT, PATCH, DELETE"s;
    }

    const auto& content_type() const
    {
        return m_content_type;
    }

    void handle(endpointstream client) // TODO: Fix exceptions!
    {
        while(client)
        {
            auto method = ""s, uri = ""s, version = ""s;
            client >> method >> uri >> version;
            slog << info << "HTTP request \"" << method << ' ' << uri << ' ' << version << "\"" << flush;

            client >> std::ws;

            auto headers = http::headers{};
            client >> headers;
            // headers object will do the logging into syslog

            client.ignore(2); // crlf

            auto content_length = 0ull;
            if(headers.count("Content-Length"))
                content_length = std::stoll(headers["Content-Length"]);

            auto authorization = ""s;
            if(headers.count("Authorization"))
                authorization = headers["Authorization"];

            auto body = std::string(content_length,' ');

            for(auto& c : body)
                c = client.get();

            slog << info << "HTTP request body \"" << body << "\"" << flush;

            if(!m_methods.count(method))
            {
                client << "HTTP/1.1 400 Bad Request"                    << crlf
                       << "Date: "         << date()                    << crlf
                       << "Server: "       << host()                    << crlf
                       << "Content-Type: " << content_type()            << crlf
                       << "Content-Length: 0"                           << crlf
                       << "Access-Control-Allow-Methods: " << methods() << crlf
                       << crlf << flush;
            }
            else if(m_authenticate && (authorization.empty() || authorization == "Basic Og=="))
            {
                client << "HTTP/1.1 401 Unauthorized status"                     << crlf
                       << "Date: "         << date()                             << crlf
                       << "Server: "       << host()                             << crlf
                       << "Content-Type: " << content_type()                     << crlf
                       << "Content-Length: 0"                                    << crlf
                       << "WWW-Authenticate: Basic realm=\"User Visible Realm\"" << crlf
                       << crlf << flush;
            }
            else if(uri == "/favicon.ico")
            {
                client << "HTTP/1.1 404 Not Found"           << crlf
                       << "Date: "         << date()         << crlf
                       << "Server: "       << host()         << crlf
                       << "Content-Type: " << content_type() << crlf
                       << "Content-Length: 0"                << crlf
                       << crlf << flush;
            }
            else if(version != "HTTP/1.1")
            {
                client << "HTTP/1.1 505 HTTP Version Not Supported" << crlf
                       << "Date: "         << date()                << crlf
                       << "Server: "       << host()                << crlf
                       << "Content-Type: " << content_type()        << crlf
                       << "Content-Length: 0"                       << crlf
                       << crlf << flush;
            }
            else
            {
                auto content = ""s, content_type = ""s;

                for(auto& route : m_router)
                    if(std::regex_match(uri,std::regex(route.first)))
                        std::tie(content,content_type) = route.second[method].render(uri,body);

                client << "HTTP/1.1 200 OK"                                           << crlf
                       << "Date: " << date()                                          << crlf
                       << "Server: " << host()                                        << crlf
                       << "Content-Type: " << content_type                            << crlf
                       << "Content-Length: " << content.length()                      << crlf
                       << "Access-Control-Allow-Methods: " << methods()               << crlf
                       << "Access-Control-Allow-Origin: http://localhost:8080"        << crlf
                       << "Access-Control-Allow-Headers: Content-Type, Authorization" << crlf
                       << "Access-Control-Allow-Credentials: true"                    << crlf
                       << crlf
                       << (method != "HEAD"s ? content : ""s) << flush;
            }
        }
    }

    using router = std::unordered_map<std::string,std::unordered_map<std::string,controller>>;

    router m_router;

    std::string m_host = syslog::hostname;

    std::string m_content_type = "*/*"s;

    std::set<std::string> m_methods = {"HEAD"s, "OPTIONS"s};

    bool m_authenticate = false;
};

} // namespace http
