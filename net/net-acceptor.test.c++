module net;
import tester;
import std;

using namespace net;

namespace {
using tester::basic::test_case;
using tester::assertions::check_eq;
using tester::assertions::check_true;
using tester::assertions::failed;

inline bool network_tests_enabled()
{
    if(const auto* v = std::getenv("NET_DISABLE_NETWORK_TESTS"))
        return std::string_view{v} != "1";
    return true;
}
}

auto acceptor_test_reg = test_case("Acceptor") = [] {
    if(!network_tests_enabled()) return;
    tester::bdd::scenario("Basic construction") = [] {
        tester::bdd::given("An acceptor for localhost:54321") = [] {
            auto ator = net::acceptor{"localhost","54321"};
            check_eq(ator.host(),"localhost");
            check_eq(ator.service_or_port(),"54321");
            check_eq(ator.timeout(),net::default_accept_timeout);
        };
    };

    tester::bdd::scenario("Accept a connection") = [] {
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
                [&]{
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
                [&]{
                    try {
                        auto s = net::connect("localhost","50001");
                        connect_ok = static_cast<bool>(s);
                    } catch (...) {
                        connect_threw = true;
                    }
                }};

            auto start = std::chrono::steady_clock::now();
            while (!accepted && (std::chrono::steady_clock::now() - start) < 5s) {
                std::this_thread::sleep_for(100ms);
            }

            if (!accepted) {
                failed("Accept test timed out");
            }

            t1.join();
            t2.join();

            check_true(!accept_threw);
            check_true(!connect_threw);
            check_true(connect_ok);
            check_true(stream_ok);
            {
                auto lock = std::lock_guard<std::mutex>{host_mutex};
                // host might be ::1 on some systems or 127.0.0.1
                check_true(host_value == "localhost" || host_value == "::1" || host_value == "127.0.0.1");
            }
        };
    };
};
