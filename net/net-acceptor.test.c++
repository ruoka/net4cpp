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
    return true;
}

const auto _ = register_acceptor_tests();
