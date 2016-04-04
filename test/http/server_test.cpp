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
<p>Hello world!</p>
</body>
</html>)"s;

auto json = u8R"( {"foo" : 1, "bar" : false} )"s;

} // namespace view

TEST(HttpServer,Setup)
{
    auto server = http::server{};

    server.get("/"s).response(view::index);

    server.get("/json"s).response(view::json);

    server.get("/vk"s).response([](){return "Kaius Ruokonen"s;});

    server.post("/vk"s).response([](){return "Kaius Ruokonen"s;});

    server.put("/"s).response(view::index);

    server.destroy("").response(view::index);

    server.start();
}
