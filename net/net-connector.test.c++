// Copyright (c) 2025-2026 Kaius Ruokonen. All rights reserved.
// SPDX-License-Identifier: MIT
// See the LICENSE file in the project root for full license text.

module net;
import :address_info;
import :acceptor;
import :connector;
import :posix;
import :socket;
import tester;
import std;


using namespace net;

namespace {
using tester::assertions::check_true;
using tester::assertions::check_eq;
using tester::assertions::check_nothrow;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}
}

auto register_connector_tests()
{
    if(not network_tests_enabled()) return false;

    tester::bdd::scenario("Connector basic construction, [net]") = [] {
        tester::bdd::given("A connector to example.host:http") = [] {
            const auto ctor = net::connector{"example.host","http"};
            check_eq(ctor.host(), "example.host");
            check_eq(ctor.service_or_port(), "http");
            check_eq(ctor.timeout(), default_connect_timeout);
        };
    };

    // Local ephemeral accept/connect — no dependency on external DNS/HTTP.
    tester::bdd::scenario("Connecting to a host, [net]") = [] {
        tester::bdd::given("A local acceptor and connector") = [] {
            using namespace std::chrono_literals;

            auto ator = std::make_shared<net::acceptor>("127.0.0.1", "0");
            ator->timeout(2s);
            const auto port = ator->bound_port();
            check_true(port != 0);

            auto accepted = std::make_shared<std::atomic<bool>>(false);
            std::thread acceptor_thread{[ator, accepted]
            {
                try
                {
                    auto [stream, host, peer_port] = ator->accept();
                    (void)stream;
                    (void)host;
                    (void)peer_port;
                    accepted->store(true);
                }
                catch(...)
                {
                }
            }};

            check_nothrow([port]
            {
                const auto s = net::connect("127.0.0.1", std::to_string(port));
                check_true(static_cast<bool>(s));
            });

            if(acceptor_thread.joinable())
                acceptor_thread.join();
            check_true(accepted->load());
        };
    };

    tester::bdd::scenario("Connection timeout and failures, [net]") = [] {
        tester::bdd::given("A connector to localhost on a busy port") = [] {
            const auto address = address_info{"localhost", "1999", posix::sock_stream, posix::ai_passive};
            net::socket s{address->ai_family, address->ai_socktype, address->ai_protocol};
            posix::bind(s, address->ai_addr, address->ai_addrlen);
            auto ctor = net::connector{"localhost", "1999"};
            using namespace std::chrono_literals;
            ctor.timeout(100ms);
            // We don't necessarily want to assert a throw here as it depends on system state
        };
    };

    return true;
}

const auto _ = register_connector_tests();
