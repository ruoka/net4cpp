// Copyright (c) 2025-2026 Kaius Ruokonen. All rights reserved.
// SPDX-License-Identifier: MIT
// See the LICENSE file in the project root for full license text.

// Compiles the README.md code examples, one test case per README section. Servers
// are configured but never listen; client code sits in lambdas that are not called.

import net;
import std;
import tester;

namespace {
using tester::basic::test_case;
using namespace std::string_literals;
using namespace std::string_view_literals;
}

auto register_readme_examples_tests()
{
    test_case("README: HTTP server, [readme]") = [] {
        using namespace tester::assertions;

        auto server = http::server{};
        server.get("/").text("Hello, World!");
        server.get("/api/status").json(R"({"status": "ok"})");

        succeed("HTTP server example compiles");
    };

    test_case("README: Request handlers, [readme]") = [] {
        using namespace tester::assertions;

        auto server = http::server{};
        server.get(R"(/orders/([0-9]+)(\?.*)?)").response("application/json",
            [](const http::request& req, http::body_view, http::headers&) {
                const auto id = req.capture(1);
                const auto sort = req.query("sort").value_or("asc");
                return http::make_response(http::status_ok,
                    std::format(R"({{"id":{},"sort":"{}","from":"{}"}})", id, sort, req.peer));
            });

        const auto route = std::regex{R"(/orders/([0-9]+)(\?.*)?)"};
        const auto target = "/orders/7?sort=desc"sv;
        check_true(std::regex_match(target.begin(), target.end(), route));

        auto hdr = http::headers{};
        auto [status, content, type, custom_headers] = server.get(R"(/orders/([0-9]+)(\?.*)?)").render(
            http::request{.method = "GET"sv, .target = target, .peer = "10.0.0.1"sv, .route = &route}, ""sv, hdr);
        check_eq(status, http::status_ok);
        check_eq(content, R"({"id":7,"sort":"desc","from":"10.0.0.1"})"s);
        check_eq(type, "application/json"s);
    };

    test_case("README: Middlewares, [readme]") = [] {
        using namespace tester::assertions;

        auto is_public = [](std::string_view target) { return target == "/health"; };
        auto validate = [](std::string_view authorization) { return authorization == "Bearer secret"; };

        const auto common = std::vector<http::request_middleware_factory>{
            http::middleware::correlation_id_middleware(),
            http::middleware::authentication_middleware(is_public, validate, "orders"),
            http::middleware::body_size_validation_middleware(64 * 1024),
        };

        auto order = [](const http::request& req, http::body_view, http::headers& hdr) {
            return http::make_response(http::status_ok,
                std::format(R"({{"id":"{}","correlation":"{}"}})", req.capture(1), hdr["x-correlation-id"]));
        };

        auto server = http::server{};
        server.get("/orders/([0-9]+)").response("application/json", http::middleware::wrap(order, common));
        server.get("/health").text("OK");

        auto hdr = http::headers{};
        auto [status, content, type, custom_headers] = server.get("/orders/([0-9]+)").render(http::request{.target = "/orders/7"sv}, ""sv, hdr);
        check_eq(status, http::status_unauthorized);
        check_true(hdr.contains("x-correlation-id"));
    };

    test_case("README: HTTP client, [readme]") = [] {
        using namespace tester::assertions;

        [[maybe_unused]] auto example = [] {
            auto res = http::client::request("127.0.0.1", "8080", "GET", "/api/status");
            if(res.ok())
                std::clog << res.status_code << ' ' << res.body << '\n';

            auto hdrs = http::headers{};
            hdrs.set("authorization", "Bearer secret");
            auto created = http::client::request("127.0.0.1", "8080", "POST", "/orders",
                                                 hdrs, R"({"item":"book"})", std::chrono::seconds{5});
            return created.ok();
        };

        [[maybe_unused]] auto raw = [] {
            using namespace net;
            auto s = connect("www.google.com", "http");
            s << "GET / HTTP/1.1"                << crlf
              << "Host: www.google.com"          << crlf
              << "Connection: close"             << crlf
              << crlf
              << flush;
        };

        succeed("HTTP client examples compile");
    };

    test_case("README: HTML and URL escaping, [readme]") = [] {
        using namespace tester::assertions;

        auto symbol = "NOKIA"s;
        auto reference = R"(ACC+"<x>)"s;

        auto contents = std::ostringstream{};
        contents << "<td>" << http::html_escaped{symbol} << "</td>";
        contents << "<a href=\"/cancel?ref=" << http::url_encoded{reference} << "\">Cancel</a>";

        auto decoded = net::url_decode("ACC%2B%22%3Cx%3E");
        check_eq(decoded, reference);
    };

    test_case("README: Server-Sent Events, [readme]") = [] {
        using namespace tester::assertions;

        auto server = http::server{};
        auto allow = [](std::string_view o) { return o == "http://localhost:3000"; };

        server.options("/events/.*").response("text/plain",
            http::middleware::cors_middleware(allow)([](auto&&, auto&&, auto&&) {
                return http::make_response(http::status_no_content, "");
            }));

        server.sse(R"(/events/(\w+))")
            .gate(http::middleware::origin_gate({"http://localhost:3000"}))
            .cors(allow)
            .sse([](http::sse::session& session, const http::request& req, http::headers& hdr) {
                if(hdr.contains("last-event-id"))
                {
                }
                session.send_comment("ok");
                session.send_event("topic", req.capture(1), "1");
            });

        check_true(server.sse(R"(/events/(\w+))").has_sse());
        check_true(server.sse(R"(/events/(\w+))").has_gate());
    };

    test_case("README: WebSocket, [readme]") = [] {
        using namespace tester::assertions;

        auto server = http::server{};
        server.ws("/echo").ws([](std::string_view msg) {
            return net::websocket::text_reply{std::string{msg}};
        });
        server.ws(R"(/rooms/(\w+))")
            .gate(http::middleware::origin_gate({"http://localhost:3000"}))
            .ws([](const http::request& req, std::string_view msg) {
                return net::websocket::text_reply{std::format("{}: {}", req.capture(1), msg)};
            });

        [[maybe_unused]] auto client = [] {
            auto ws = net::websocket::connect("127.0.0.1", "8080", "/echo");
            if(ws.send("hello"))
            {
                if(auto reply = ws.recv())
                {
                }
            }
            ws.close();
        };

        check_true(server.ws("/echo").has_websocket());
        check_true(server.ws(R"(/rooms/(\w+))").has_gate());
    };

    test_case("README: MCP over SSE, [readme]") = [] {
        using namespace tester::assertions;

        auto http = http::server{};
        auto mcp = http::mcp::server{"/messages/"};
        mcp.info({.name = "demo", .version = "1.0.0"})
            .list_tools([] {
                return std::vector<http::mcp::tool_spec>{
                    {.name = "health", .description = "liveness"},
                };
            })
            .call_tool([](std::string_view, std::string_view) {
                return http::mcp::tool_result{.text = "ok"};
            })
            .attach(http, "/sse");

        check_true(http.sse("/sse").has_sse());
    };

    test_case("README: Syslog stream, [readme]") = [] {
        using namespace net;
        using tester::assertions::succeed;

        slog.log_level(syslog::severity::info);
        slog.facility(syslog::facility::local0);
        slog.app_name("example");

        auto clothes = "shirts"s; auto spouse = "wife"; auto wrong = false;

        slog << debug   << "... " << 3 << ' ' << 2 << ' ' << 1 << " Liftoff" << flush;
        slog << info    << "The papers want to know whose " << clothes << " you wear..." << flush;
        slog << notice  << "Tell my " << spouse << " I love her very much!" << flush;
        slog << net::warning << "Ground Control to Major Tom Your circuit's dead, there's something " << wrong << '?' << flush;
        slog << error   << "Planet Earth is blue and there's nothing I can do." << flush;

        [[maybe_unused]] auto fluent = [] {
            slog.format(net::log_format::jsonl)
                .app_name("my-service")
                .log_level(syslog::severity::info)
                .sd_id("app")
                .redirect("logs/app.log");
        };

        slog << info << std::pair{"user_id", 42} << std::pair{"ip", "192.168.1.1"} << "User logged in" << flush;

        slog << info << std::source_location::current() << "Operation completed" << flush;
        slog << error
             << std::source_location::current()
             << std::pair{"duration_ms", 127}
             << "Request failed"
             << flush;

        if(slog.is_enabled(syslog::severity::debug))
        {
            auto details = "debug info"s;
            slog << debug << details << flush;
        }

        succeed("Syslog stream examples compile");
    };

    return true;
}

const auto _ = register_readme_examples_tests();
