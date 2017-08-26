#pragma once

#include <functional>
#include <thread>
#include <unordered_map>
#include "std/extension.hpp"
#include "net/acceptor.hpp"

namespace http {

    using namespace std::string_literals;
    using namespace std::chrono_literals;

    class controller
    {
    public:

        using callback = std::function<const std::string&()>;

        void response(const std::string& view)
        {
            m_callback = [&](){return view;};
        }

        void response(callback cb)
        {
            m_callback = cb;
        }

        std::string render()
        {
            return m_callback();
        }

    private:
        callback m_callback = [](){return "HTTP/1.1 404 Not Found"s;};
    };

    class server
    {
    public:

        controller& get(const std::string& path)
        {
            return m_router[path]["GET"s];
        }

        controller& head(const std::string& path)
        {
            return m_router[path]["HEAD"s];
        }

        controller& post(const std::string& path)
        {
            return m_router[path]["POST"s];
        }

        controller& put(const std::string& path)
        {
            return m_router[path]["PUT"s];
        }

        controller& patch(const std::string& path)
        {
            return m_router[path]["PATCH"s];
        }

        controller& destroy(const std::string& path)
        {
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

        void handle(net::endpointstream client)
        {
            using namespace std;

            while(client)
            {
                auto method = ""s, uri = ""s, version = ""s;
                client >> method >> uri >> version;

                clog << method << ' ' << uri << ' ' << version << net::newl;

                client >> ws;

                auto content_type = "text/html"s, authorization = ""s;

                while(client && client.peek() != '\r')
                {
                    auto name = ""s, value = ""s;
                    getline(client, name, ':');
                    ext::trim(name);
                    getline(client, value);
                    ext::trim(value);
                    clog << name << ": " << value << endl;

                    if(name == "Accept")
                        content_type = value;

                    if(name == "Authorization")
                        authorization = value;
                }
                client.ignore(2); // crlf

                if(uri == "/favicon.ico")
                {
                    client << "HTTP/1.1 404 Not Found"                                 << net::crlf
                           << "Date: " << ext::to_rfc1123(chrono::system_clock::now()) << net::crlf
                           << "Server: net4cpp/1.1"                                    << net::crlf
                           << "Content-Type: " << content_type                         << net::crlf
                           << "Content-Length: 0"                                      << net::crlf
                           << net::crlf << net::flush;
                }
                else if(m_authenticate && (authorization.empty() || authorization == "Basic Og=="))
                {
                    client << "HTTP/1.1 401 Unauthorized status"                       << net::crlf
                           << "Date: " << ext::to_rfc1123(chrono::system_clock::now()) << net::crlf
                           << "Server: net4cpp/1.1"                                    << net::crlf
                           << "WWW-Authenticate: Basic realm=\"User Visible Realm\""   << net::crlf
                           << "Content-Type: " << content_type                         << net::crlf
                           << "Content-Length: 0"                                      << net::crlf
                           << net::crlf << net::flush;
                }
                else
                {
                  const auto& content = m_router[uri][method].render();

                  client << "HTTP/1.1 200 OK"                                                   << net::crlf
                         << "Date: " << ext::to_rfc1123(chrono::system_clock::now())            << net::crlf
                         << "Server: net4cpp/1.1"                                               << net::crlf
                         << "Access-Control-Allow-Origin: *"                                    << net::crlf
                         << "Access-Control-Allow-Methods: HEAD, GET, POST, PUT, PATCH, DELETE" << net::crlf
                         << "Access-Control-Allow-Headers: Content-Type"                        << net::crlf
                         << "Access-Control-Allow-Credentials: true"                            << net::crlf
                         << "Content-Type: " << content_type                                    << net::crlf
                         << "Content-Length: " << content.length()                              << net::crlf
                         << net::crlf
                         << (method != "HEAD"s ? content : ""s) << net::flush;
                }
            }
        }

        using router = std::unordered_map<std::string,std::unordered_map<std::string,controller>>;

        router m_router;

        bool m_authenticate = false;
    };

} // namespace http
