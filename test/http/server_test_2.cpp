#include <gtest/gtest.h>
#include "http/server.hpp"

using namespace std::string_literals;

TEST(HttpServerTest2,TestRegexRoutes)
{
    auto server = http::server{};

    auto handler = [](const std::string& uri,const std::string& body){return "<p>"s + uri + "</p>"s;};

    server.get("/[a-z]*").response("text/html",handler);

    server.get("/[a-z]+/[0-9]+").response("text/html",handler);

    server.listen("8080");
}
