#pragma once

#include <functional>
#include <thread>
#include <unordered_map>
#include "std/extension.hpp"
#include "net/acceptor.hpp"

using namespace std;
using namespace chrono;
using namespace string_literals;
using namespace net;

namespace http {

    class controller
    {
    public:

        using callback = function<const string&()>;

        void response(const string& view)
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

        controller& get(const string& path)
        {
            return m_router[path]["GET"s];
        }

        controller& head(const string& path)
        {
            return m_router[path]["HEAD"s];
        }

        controller& post(const string& path)
        {
            return m_router[path]["POST"s];
        }

        controller& put(const string& path)
        {
            return m_router[path]["PUT"s];
        }

        controller& patch(const string& path)
        {
            return m_router[path]["PATCH"s];
        }

        controller& destroy(const string& path)
        {
            return m_router[path]["DELETE"s];
        }

        void listen(const std::string& serice_or_port = "http"s)
        {
            auto acceptor = net::acceptor{"localhost"s, serice_or_port};
            acceptor.timeout(1h);
            while(true)
            {
                auto connection = acceptor.accept();
                auto worker = thread{[&](){handle(std::move(connection));}};
                worker.detach();
            }
        }

    private:

        void handle(net::endpointstream connection)
        {
            while(connection)
            {
                auto method = ""s, uri = ""s, version = ""s;
                connection >> method >> uri >> version;

                clog << method << ' ' << uri << ' ' << version;

                connection >> ws;

                while(connection && connection.peek() != '\r')
                {
                    auto name = ""s, value = ""s;
                    getline(connection, name, ':');
                    trim(name);
                    getline(connection, value);
                    trim(value);
                    clog << name << ": " << value << endl;
                }

                const auto content = m_router[uri][method].render();

                connection << "HTTP/1.1 200 OK"                                                   << crlf
                           << "Date: " << to_rfc1123(system_clock::now())                         << crlf
                           << "Server: YARESTDB/0.1"                                              << crlf
                           << "Access-Control-Allow-Origin: *"                                    << crlf
                           << "Access-Control-Allow-Methods: HEAD, GET, POST, PUT, PATCH, DELETE" << crlf
                           << "Content-Type: text/html; charset=UTF-8"                            << crlf
                           << "Content-Length: " << content.length()                              << crlf
                           << crlf
                           << (method != "HEAD"s ? content : ""s) << flush;
            }
        }

        using router = unordered_map<string,unordered_map<string,controller>>;

        router m_router;
    };

} // namespace http
