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

        void listen(const std::string& serice_or_port = "http"s)
        {
            auto acceptor = net::acceptor{"localhost"s, serice_or_port};
            while(true)
            {
                auto client = acceptor.accept();
                while(client)
                {
                    auto method = ""s, path = ""s, line = "xxx"s;
                    client >> method >> path;
                    clog << method << ' ' << path << ' ';

                    while(client && line.length() > 2)
                    {
                        getline(client, line);
                        clog << line << endl;
                    }

                    auto content = m_router[path][method].render();

                    client << "HTTP/1.1 200 OK\r\n"
                           << "Date: Mon, 11 Apr 2016 22:10:17 GMT\r\n"s
                           << "Server: http://localhost:8080/vk\r\n"
                           << "Access-Control-Allow-Origin: *\r\n"
                           << "Access-Control-Allow-Methods: POST, GET, PUT, DELETE\r\n"
                           << "Content-Type: text/html; charset=UTF-8\r\n"
                           << "Content-Length: " << content.length() << "\r\n"
                           << "\r\n"
                           << content << flush;
                }
            }
        }

    private:

        using router = unordered_map<string,unordered_map<string,controller>>;

        router m_router;
    };

} // namespace http
