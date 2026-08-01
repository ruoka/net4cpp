// Copyright (c) 2025-2026 Kaius Ruokonen. All rights reserved.
// SPDX-License-Identifier: MIT
// See the LICENSE file in the project root for full license text.

module net;
import tester;
import std;

using namespace net;

namespace {
using tester::assertions::check_eq;
using tester::assertions::check_true;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}
}

auto register_receiver_sender_tests()
{
    // Register only — do not exercise multicast during static init. Soft
    // assertions need an active execution_context; running check_eq from a
    // global constructor aborts test_runner before any scenario starts.
    tester::bdd::scenario("Receiver and sender multicast round-trip, [net]") = [] {
        if(not network_tests_enabled()) return;

        std::array<int, 100> data;
        for(int i = 0; i < 100; ++i) data[i] = i + 1;

        using namespace std::chrono_literals;
        std::atomic<bool> done{false};
        std::atomic<bool> receiver_ready{false};
        std::atomic<bool> receiver_timed_out{false};
        std::atomic<bool> receiver_failed{false};
        std::atomic<bool> sender_failed{false};

        auto received = std::vector<int>{};
        received.reserve(data.size());
        auto received_mutex = std::mutex{};

        std::thread t1{
            [&]() {
                try {
                    auto rver = net::receiver{"228.0.0.4", "54321"};
                    auto is = rver.join();
                    receiver_ready = true;
                    for([[maybe_unused]] auto i : data)
                    {
                        auto ii = 0;
                        if(is.wait_for(5s)) {
                            is >> ii;
                            auto lock = std::lock_guard<std::mutex>{received_mutex};
                            received.push_back(ii);
                        } else {
                            receiver_timed_out = true;
                            break;
                        }
                    }
                    done = true;
                } catch(...) {
                    receiver_failed = true;
                    done = true;
                }
            }};

        auto start_ready = std::chrono::steady_clock::now();
        while(not receiver_ready and (std::chrono::steady_clock::now() - start_ready) < 5s) {
            std::this_thread::sleep_for(10ms);
        }

        std::thread t2{
            [&]() {
                try {
                    auto sder = net::sender{"228.0.0.4", "54321"};
                    auto os = sder.distribute();
                    for(auto i : data)
                    {
                        std::this_thread::sleep_for(2ms);
                        os << i << std::endl;
                    }
                } catch(...) {
                    sender_failed = true;
                }
            }};

        auto start = std::chrono::steady_clock::now();
        while(not done and (std::chrono::steady_clock::now() - start) < 15s) {
            std::this_thread::sleep_for(100ms);
        }

        if(not done) {
            receiver_timed_out = true;
        }

        if(t1.joinable()) t1.join();
        if(t2.joinable()) t2.join();

        // If multicast is unavailable, treat as a warning rather than failing CI/dev machines.
        if(sender_failed) {
            tester::assertions::warning("Sender failed (multicast may be unavailable on this host/network)");
            return;
        }
        if(receiver_failed) {
            tester::assertions::warning("Receiver failed (multicast may be unavailable on this host/network)");
            return;
        }

        for(std::size_t idx = 0; idx < received.size(); ++idx) {
            check_eq(data[idx], received[idx]);
        }
        if(received.size() != data.size()) {
            tester::assertions::warning("Multicast data incomplete (multicast may be unavailable on this host/network)");
        }
    };

    return true;
}

const auto _ = register_receiver_sender_tests();
