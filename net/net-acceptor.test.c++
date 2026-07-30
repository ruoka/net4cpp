// Copyright (c) 2025-2026 Kaius Ruokonen. All rights reserved.
// SPDX-License-Identifier: MIT
// See the LICENSE file in the project root for full license text.

module;
#include <csignal>
module net;
import :acceptor;
import tester;
import std;

using namespace net;

namespace {
using tester::basic::test_case;
using tester::assertions::check_eq;
using tester::assertions::check_true;
using tester::assertions::check_false;
using tester::assertions::failed;
using namespace std::string_view_literals;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}

// Counts SIGPIPE deliveries so a write-after-peer-close must not kill the process.
std::atomic<int> g_sigpipe_count{0};

void on_sigpipe(int) noexcept
{
    g_sigpipe_count.fetch_add(1, std::memory_order_relaxed);
}
}

auto register_acceptor_tests()
{
    tester::bdd::scenario("is_wildcard_bind_host covers all-interfaces aliases, [net]") = [] {
        check_true(is_wildcard_bind_host("0.0.0.0"sv));
        check_true(is_wildcard_bind_host("::"sv));
        check_true(is_wildcard_bind_host("::0"sv));
        check_true(is_wildcard_bind_host("[::]"sv));
        check_true(is_wildcard_bind_host("[::0]"sv));
        check_true(is_wildcard_bind_host("[0.0.0.0]"sv));
        check_true(is_wildcard_bind_host("[]"sv));
        check_true(is_wildcard_bind_host("  [::0]  "sv));
        check_true(is_wildcard_bind_host("0"sv));
        check_true(is_wildcard_bind_host("0.0.0"sv));
        check_true(is_wildcard_bind_host("00.0.0.0"sv));
        // "*" is accepted as ANY on some Linux getaddrinfo builds, not on Darwin.
        check_true(is_wildcard_bind_host("0::"sv));
        check_true(is_wildcard_bind_host("0000::"sv));
        check_true(is_wildcard_bind_host("0::0"sv));
        check_true(is_wildcard_bind_host("::0000"sv));
        check_true(is_wildcard_bind_host("0:0:0:0:0:0:0:0"sv));
        check_true(is_wildcard_bind_host("[0::]"sv));
        check_true(is_wildcard_bind_host("[0:0:0:0:0:0:0:0]"sv));
        check_true(is_wildcard_bind_host("::ffff:0.0.0.0"sv));
        check_true(is_wildcard_bind_host("[::ffff:0.0.0.0]"sv));

        check_false(is_wildcard_bind_host("127.0.0.1"sv));
        check_false(is_wildcard_bind_host("::1"sv));
        check_false(is_wildcard_bind_host("[::1]"sv));
        check_false(is_wildcard_bind_host("localhost"sv));
    };

    if(not network_tests_enabled()) return true;

    tester::bdd::scenario("Accept times out with no client, [net]") = [] {
        using namespace std::chrono_literals;
        auto ator = net::acceptor{"127.0.0.1", "0"};
        ator.timeout(200ms);
        const auto start = std::chrono::steady_clock::now();
        auto timed_out = false;
        try
        {
            (void)ator.accept();
        }
        catch(const std::system_error& e)
        {
            timed_out = std::string_view{e.what()}.contains("timeout");
        }
        check_true(timed_out);
        check_true(std::chrono::steady_clock::now() - start < 2s);
    };

    tester::bdd::scenario("close wakes accept wait promptly, [net]") = [] {
        using namespace std::chrono_literals;
        auto ator = std::make_shared<net::acceptor>("127.0.0.1", "0");
        // Long timeout: without close()-wake this would sit ~30s.
        ator->timeout(30s);
        auto accept_error = std::make_shared<std::optional<std::system_error>>();
        std::promise<void> entered;
        auto entered_future = entered.get_future();

        std::thread t{[ator, accept_error, &entered] {
            entered.set_value();
            try
            {
                (void)ator->accept();
            }
            catch(const std::system_error& e)
            {
                *accept_error = e;
            }
        }};

        check_true(entered_future.wait_for(2s) == std::future_status::ready);
        std::this_thread::sleep_for(50ms);
        const auto start = std::chrono::steady_clock::now();
        ator->close();
        t.join();
        const auto elapsed = std::chrono::steady_clock::now() - start;

        check_true(ator->closed());
        check_true(accept_error->has_value());
        check_true(
            (*accept_error)->code() == std::errc::bad_file_descriptor
            or std::string_view{(*accept_error)->what()}.contains("acceptor closed"));
        // Bound proves wake (not the 30s accept timeout); allow scheduler noise.
        check_true(elapsed < 2s);
    };

    tester::bdd::scenario("Basic construction, [net]") = [] {
        tester::bdd::given("An acceptor for localhost:54321") = [] {
            auto ator = net::acceptor{"localhost","54321"};
            check_eq(ator.host(),"localhost");
            check_eq(ator.service_or_port(),"54321");
            check_eq(ator.timeout(),net::default_accept_timeout);
        };
    };

    tester::bdd::scenario("Accept a connection, [net]") = [] {
        tester::bdd::given("An acceptor and a connector") = [] {
            using namespace std::chrono_literals;
            std::atomic<bool> accepted{false};
            std::atomic<bool> accept_threw{false};
            std::atomic<bool> connect_ok{false};
            std::atomic<bool> connect_threw{false};
            std::atomic<bool> stream_ok{false};
            auto host_value = std::string{};
            auto host_mutex = std::mutex{};

            std::thread t1{
                [&]() {
                    try {
                        auto ator = net::acceptor{"localhost","50001"};
                        ator.timeout(2s);
                        auto [stream, host, port] = ator.accept();
                        stream_ok = static_cast<bool>(stream);
                        {
                            auto lock = std::lock_guard<std::mutex>{host_mutex};
                            host_value = host;
                        }
                        accepted = true;
                    } catch (...) {
                        accept_threw = true;
                    }
                }};

            std::this_thread::sleep_for(100ms);

            std::thread t2{
                [&]() {
                    try {
                        auto s = net::connect("localhost","50001");
                        connect_ok = static_cast<bool>(s);
                    } catch (...) {
                        connect_threw = true;
                    }
                }};

            auto start = std::chrono::steady_clock::now();
            while (not accepted and (std::chrono::steady_clock::now() - start) < 5s) {
                std::this_thread::sleep_for(100ms);
            }

            if (not accepted) {
                failed("Accept test timed out");
            }

            t1.join();
            t2.join();

            check_true(not accept_threw);
            check_true(not connect_threw);
            check_true(connect_ok);
            check_true(stream_ok);
            {
                auto lock = std::lock_guard<std::mutex>{host_mutex};
                // host might be ::1 on some systems or 127.0.0.1
                check_true(host_value == "localhost" or host_value == "::1" or host_value == "127.0.0.1");
            }
        };
    };

    // Regression: on macOS MSG_NOSIGNAL is 0, so accepted sockets without
    // SO_NOSIGPIPE raise SIGPIPE (process death) when writing after the peer
    // closes. Linux is covered by MSG_NOSIGNAL; macOS needs the accept-path
    // setsockopt that connector already applies on outbound sockets.
    tester::bdd::scenario("accepted write after peer close does not raise SIGPIPE, [net]") = [] {
        using namespace std::chrono_literals;
        g_sigpipe_count.store(0, std::memory_order_relaxed);
        const auto previous = std::signal(SIGPIPE, on_sigpipe);

        auto ator = net::acceptor{"127.0.0.1", "0"};
        ator.timeout(2s);
        const auto port = ator.bound_port();
        check_true(port != 0);

        std::promise<void> peer_closed;
        auto peer_closed_future = peer_closed.get_future();
        std::atomic<bool> accept_failed{false};
        std::atomic<bool> wrote{false};

        std::thread server{[&] {
            try
            {
                auto [stream, host, client_port] = ator.accept();
                (void)host;
                (void)client_port;
                check_true(peer_closed_future.wait_for(2s) == std::future_status::ready);
                stream << "hello" << std::flush;
                wrote = true;
            }
            catch(...)
            {
                accept_failed = true;
                try { peer_closed.set_value(); } catch(...) {}
            }
        }};

        std::this_thread::sleep_for(50ms);
        try
        {
            auto client = net::connect("127.0.0.1", std::to_string(port));
            client.close();
            peer_closed.set_value();
        }
        catch(...)
        {
            accept_failed = true;
            try { peer_closed.set_value(); } catch(...) {}
        }

        server.join();
        std::signal(SIGPIPE, previous);

        check_true(not accept_failed);
        check_true(wrote);
        check_eq(g_sigpipe_count.load(std::memory_order_relaxed), 0);
    };

    return true;
}

const auto _ = register_acceptor_tests();
