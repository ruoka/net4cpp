#include <ostream>
#include <functional>
#include <unordered_map>
#include "net/acceptor.hpp"

using namespace std;
using namespace string_literals;

namespace http {

    class renderer
    {
    public:

        using callback = function<string()>;

        void response(callback cb)
        {
            m_callback = cb;
        }

        void response(const string& view)
        {
            m_callback = [&view](){return view;};
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

        renderer& get(const string& path)
        {
            return m_path["GET"s][path];
        }

        renderer& post(const string& path)
        {
            return m_path["POST"s][path];
        }

        renderer& put(const string& path)
        {
            return m_path["PUT"s][path];
        }

        renderer& destroy(const string& path)
        {
            return m_path["DELETE"s][path];
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
                       << m_path[method][path].render() << "\r\n\r\n" << flush;
            }
        }

    private:

        unordered_map<string,unordered_map<string,renderer>> m_path;
    };

} // namespace http
