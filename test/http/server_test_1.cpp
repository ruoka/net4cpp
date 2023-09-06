#include <gtest/gtest.h>
#include "http/server.hpp"

namespace view {

const auto index =
R"(

<!DOCTYPE html>
<html>
<head>
    <title>MVC++</title>
    <style type="text/css">
    </style>
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/2.2.2/jquery.min.js"></script>
    <script type="text/javascript">
    $(document).ready(function() {
        $("button").click( function() {
            $.ajax({url : $(this).attr("data-url"), method : $(this).attr("data-method")})
            .done(function(data) {$("#content").html(data)})
            .fail(function (jqXHR, textStatus, errorThrown) {alert(textStatus + ":" + jqXHR.status)})
        })
    })
    </script>
</head>
<body>
    <aside>
        <button type="button" data-url="/vk" data-method="GET"   > GET</button>
        <button type="button" data-url="/vk" data-method="HEAD"  > HEAD</button>
        <button type="button" data-url="/vk" data-method="POST"  > POST</button>
        <button type="button" data-url="/vk" data-method="PUT"   > PUT</button>
        <button type="button" data-url="/vk" data-method="PATCH" > PATCH</button>
        <button type="button" data-url="/vk" data-method="DELETE"> DELETE</button>
        <button type="button" data-url="/json" data-method="GET" > JSON</button>
    </aside>
    <article id = "content"></article>
</body>
</html>

)";

const auto get = R"(<p>GET response</p>)";

const auto head = R"(<p>HEAD response</p>)";

const auto post = R"(<p>POST response</p>)";

const auto put = R"(<p>PUT response</p>)";

const auto destroy = R"(<p>DELETE response</p>)";

const auto patch = R"(<p>PATCH response</p>)";

// const auto json = R"({"foo" : 1, "bar" : false})";

const auto json = std::string{"{\"foo\" : 1, \"bar\" : false}"};

} // namespace view

TEST(HttpServerTest1,TestMethodds)
{
    auto server = http::server{};

    server.get("/").html(view::index);
    server.get("/vk").html(view::get);
    server.head("/vk").html(view::head);
    server.post("/vk").html(view::post);
    server.put("/vk").html(view::put);
    server.patch("/vk").html(view::patch);
    server.destroy("/vk").html(view::destroy);
    server.get("/json").json(view::json);
    server.ws("/").json(view::json);

    server.listen("8080");
}
