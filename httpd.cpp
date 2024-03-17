#include <fstream>
#include "http/server.hpp"
using namespace std::literals;

int main(int argc, char** argv)
{
    constexpr auto eof = static_cast<char>(std::char_traits<char>::eof());
    const auto root = argc > 1 ? std::string_view{argv[1]} : "."sv;
    net::slog.level(net::syslog::severity::info);
    net::slog.redirect(std::clog);
    net::slog << net::info("httpd") << "Staring httpd server root=" << root<< net::flush;
    auto httpd = http::server{};

    auto handler = [root](auto request, auto, const auto& headers)
        {
            const auto file = std::string{root} += request;
            net::slog << net::info("httpd") << "responding " << file << net::flush;
            auto stream = std::ifstream{file};
            auto content = std::string{};
            std::getline(stream,content,eof);
            return content;
        };

    httpd.get(R"(^\/.+\.html$)").response("text/html",handler);
    httpd.get(R"(^\/.+\.css$)").response("text/css",handler);
    httpd.get(R"(^\/.+\.js$)").response("text/javascript",handler);
    httpd.listen("8080");
    return 0;
}
