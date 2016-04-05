#include <ostream>
#include <functional>
#include <unordered_map>
#include "net/acceptor.hpp"

using namespace std;
using namespace string_literals;

namespace http {

    class controller
    {
    public:

        using callback = function<string()>;

        void response(const string& view)
        {
            m_callback = [&view](){return view;};
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

        controller& post(const string& path)
        {
            return m_router[path]["POST"s];
        }

        controller& put(const string& path)
        {
            return m_router[path]["PUT"s];
        }

        controller& destroy(const string& path)
        {
            return m_router[path]["DELETE"s];
        }

        void start()
        {
            auto acceptor = net::acceptor{"localhost"s, "8080"s};
            while(true)
            {
                auto client = acceptor.accept();
                auto method = ""s, path = ""s, line = "xxx"s;
                client >> method >> path;
                clog << method << ' ' << path << ' ';
                while(client && line.length() > 1)
                {
                    getline(client, line);
                    clog << line << endl;
                }
                client << "HTTP/1.1 200 OK\r\n"
                       << "Content-Type: text/html; charset=UTF-8\r\n"
                       << "Location: http://localhost:8080\r\n"
                       << "Date: Mon, 04 Apr 2016 21:10:17 GMT\r\n"s
                       << "Connection: close\r\n\r\n"
                       << m_router[path][method].render() << "\r\n\r\n" << flush;
            }
        }

    private:

        using router = unordered_map<string,unordered_map<string,controller>>;

        router m_router;
    };

} // namespace http
