// Copyright (c) 2025-2026 Kaius Ruokonen. All rights reserved.
// SPDX-License-Identifier: MIT
// See the LICENSE file in the project root for full license text.

module net;
import tester;
import std;

using namespace net;

namespace {
using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;
using tester::basic::test_case;
using tester::assertions::require_eq;
using tester::assertions::require_true;
using tester::assertions::require_throws;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}

// std::thread::~thread calls terminate if still joinable — any require_* throw
// before an explicit join would abort the whole --jobs>1 run. Stop + join here.
struct joining_server_thread
{
    std::shared_ptr<http::server> server;
    std::thread thread;

    joining_server_thread() = default;
    joining_server_thread(std::shared_ptr<http::server> s, std::thread t)
        : server{std::move(s)}
        , thread{std::move(t)}
    {
    }

    joining_server_thread(joining_server_thread&& other) noexcept
        : server{std::move(other.server)}
        , thread{std::move(other.thread)}
    {
    }

    joining_server_thread& operator=(joining_server_thread&& other) noexcept
    {
        if(this != &other)
        {
            stop_and_join();
            server = std::move(other.server);
            thread = std::move(other.thread);
        }
        return *this;
    }

    ~joining_server_thread() { stop_and_join(); }

    joining_server_thread(const joining_server_thread&) = delete;
    joining_server_thread& operator=(const joining_server_thread&) = delete;

private:
    void stop_and_join() noexcept
    {
        if(server)
        {
            try { server->stop(); } catch(...) {}
        }
        if(thread.joinable())
        {
            try { thread.join(); } catch(...) {}
        }
    }
};

inline std::pair<joining_server_thread, std::uint16_t> listen_ephemeral(
    const std::shared_ptr<http::server>& server)
{
    auto port = std::make_shared<std::uint16_t>(0);
    auto bound = std::make_shared<std::promise<void>>();
    auto bound_future = bound->get_future();
    std::thread t{[server, port, bound]
    {
        try
        {
            server->listen("127.0.0.1"sv, "0"sv, [server, port, bound]
            {
                *port = server->bound_port();
                try { bound->set_value(); } catch(...) {}
            });
        }
        catch(...)
        {
            try { bound->set_value(); } catch(...) {}
        }
    }};
    if(bound_future.wait_for(3s) != std::future_status::ready)
        return {joining_server_thread{server, std::move(t)}, 0};
    return {joining_server_thread{server, std::move(t)}, *port};
}

} // namespace

auto register_http_client_tests()
{
    test_case("http client round-trip GET and POST, [net][http-client]") = []
    {
        if(not network_tests_enabled())
            return;

        auto server = std::make_shared<http::server>();
        server->get("/ping").json(R"({"ok":true})"s);
        server->post("/echo").response(
            "application/json"sv,
            [](auto, auto body, auto&)
            {
                return http::make_response(http::status_ok, std::string{body});
            });

        auto [listener, port] = listen_ephemeral(server);
        require_true(port != 0);
        const auto port_s = std::to_string(port);

        {
            auto res = http::client::request("127.0.0.1"sv, port_s, "GET"sv, "/ping"sv);
            require_true(res.ok());
            require_eq(res.status_code, 200);
            require_true(res.body.contains("ok"sv));
        }

        {
            auto hdrs = http::headers{};
            hdrs.set("authorization"s, "Basic Zm9vOmJhcg=="s);
            auto res = http::client::request(
                "127.0.0.1"sv, port_s, "POST"sv, "/echo"sv, hdrs, R"({"n":1})"sv);
            require_true(res.ok());
            require_eq(res.body, R"({"n":1})"s);
        }
    };

    test_case("http client fail-fast on bad args and missing peer, [net][http-client]") = []
    {
        require_throws([]
        {
            (void)http::client::request(""sv, "80"sv, "GET"sv, "/"sv);
        });
        if(not network_tests_enabled())
            return;
        require_throws([]
        {
            (void)http::client::request(
                "127.0.0.1"sv, "1"sv, "GET"sv, "/"sv, {}, {}, 200ms);
        });
    };

    test_case("http client io_timeout bounds the wait for the response, [net][http-client]") = []
    {
        if(not network_tests_enabled())
            return;

        auto server = std::make_shared<http::server>();
        server->get("/slow").response(
            "application/json"sv,
            [](auto, auto, auto&)
            {
                std::this_thread::sleep_for(600ms);
                return http::make_response(http::status_ok, R"({"ok":true})"s);
            });

        auto [listener, port] = listen_ephemeral(server);
        require_true(port != 0);
        const auto port_s = std::to_string(port);

        // Gives up before the server answers.
        {
            const auto start = std::chrono::steady_clock::now();
            auto error = ""s;
            try
            {
                (void)http::client::request("127.0.0.1"sv, port_s, "GET"sv, "/slow"sv, {}, {}, 1s, 150ms);
            }
            catch(const std::runtime_error& e)
            {
                error = e.what();
            }
            require_true(error.contains("timed out"sv));
            require_true(std::chrono::steady_clock::now() - start < 500ms);
        }

        // A longer io_timeout, or none, waits for the answer.
        require_true(http::client::request("127.0.0.1"sv, port_s, "GET"sv, "/slow"sv, {}, {}, 1s, 3s).ok());
        require_true(http::client::request("127.0.0.1"sv, port_s, "GET"sv, "/slow"sv).ok());
    };

    return 0;
}

const auto _ = register_http_client_tests();
