// Copyright (c) 2025-2026 Kaius Ruokonen. All rights reserved.
// SPDX-License-Identifier: MIT
// See the LICENSE file in the project root for full license text.

module net;
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

auto register_receiver_tests()
{
    if(not network_tests_enabled()) return false;

    tester::bdd::scenario("Basic construction, [net]") = [] {
        tester::bdd::given("A receiver for group 228.0.0.4 and service test") = [] {
            auto rver = net::receiver{"228.0.0.4","test"};
            check_eq(rver.group(),"228.0.0.4");
            check_eq(rver.service(),"test");
        };
    };

tester::bdd::scenario("Join multicast group, [net]") = [] {
        tester::bdd::given("Joining group 228.0.0.4 on port 54321") = [] {
            check_nothrow([] {
                auto s = net::join("228.0.0.4", "54321");
                check_true(not(not s));
            });
        };
    };

    return true;
}

const auto _ = register_receiver_tests();
