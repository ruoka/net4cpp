#include <gtest/gtest.h>
#include "http/server.hpp"

using namespace std::string_literals;

TEST(HttpServerTest2,TestRegexRoutes)
{
    auto server = http::server{};

    auto handler = []([[maybe_unused]]std::string_view request,
                      [[maybe_unused]]std::string_view body,
                      [[maybe_unused]]http::headers headers)
        {return "<p>"s + std::string{request} + "</p>"s;};

    server.get("/[a-z]*").response("text/html",handler);

    server.get("/[a-z]+/[0-9]+").response("text/html",handler);

    server.listen("8080");
}
