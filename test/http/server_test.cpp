#include <gtest/gtest.h>
#include "http/server.hpp"

namespace view {

auto index =
u8R"(<!DOCTYPE html>
<html>
<head>
<title>Test</title>
</head>
<body>
<p>index.html</p>
</body>
</html>)"s;

auto json = u8R"( {"foo" : 1, "bar" : false} )"s;

auto post = u8R"(<p>POST response</p>)"s;

auto put = u8R"(<p>PUT response</p>)"s;

auto destroy = u8R"(<p>DELETE response</p>)"s;

} // namespace view

TEST(HttpServer,Setup)
{
    auto server = http::server{};

    server.get("/"s).response(view::index);

    server.get("/json"s).response(view::json);

    server.get("/vk"s).response([](){return "Kaius Ruokonen"s;});

    server.post("/vk"s).response(view::post);

    server.put("/"s).response(view::put);

    server.destroy("/"s).response(view::destroy);

    server.start();
}
