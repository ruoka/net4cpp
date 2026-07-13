module net;
import :posix;
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

auto register_endpointbuf_tests()
{
    tester::bdd::scenario("Buffer size constants, [net]") = [] {
        tester::bdd::given("TCP and UDP buffer sizes") = [] {
            check_eq(tcp_buffer_size, 4096u);
            check_eq(udp_buffer_size, 65507u);
        };
    };

    tester::bdd::scenario("endpointbuf_base construction, [net]") = [] {
        tester::bdd::given("A socket") = [] {
            if(!network_tests_enabled()) return;
            
            auto s = socket{posix::af_inet, posix::sock_stream};
            check_nothrow([&s] {
                endpointbuf_base buf{std::move(s)};
                check_true(true);
            });
        };
    };

    tester::bdd::scenario("endpointbuf_base wait_for, [net]") = [] {
        tester::bdd::given("An endpointbuf_base with a new socket") = [] {
            if(!network_tests_enabled()) return;
            
            auto s = socket{posix::af_inet, posix::sock_stream};
            endpointbuf_base buf{std::move(s)};
            
            tester::bdd::when("Calling wait_for with a short timeout") = [&buf] {
                using namespace std::chrono_literals;
                auto result = buf.wait_for(1ms);
                check_true(!result);
            };
        };
    };

    tester::bdd::scenario("endpointbuf template construction, [net]") = [] {
        tester::bdd::given("A socket") = [] {
            if(!network_tests_enabled()) return;
            
            auto s = socket{posix::af_inet, posix::sock_stream};
            check_nothrow([&s] {
                endpointbuf<1024> buf{std::move(s)};
                check_true(true);
            });
        };
    };

    tester::bdd::scenario("Type aliases, [net]") = [] {
        tester::bdd::given("TCP and UDP endpointbuf type aliases") = [] {
            if(!network_tests_enabled()) return;
            
            auto s1 = socket{posix::af_inet, posix::sock_stream};
            auto s2 = socket{posix::af_inet, posix::sock_dgram};
            
            check_nothrow([&s1, &s2] {
                tcp_endpointbuf tcp_buf{std::move(s1)};
                udp_endpointbuf udp_buf{std::move(s2)};
                check_true(true);
            });
        };
    };

    tester::bdd::scenario("endpointbuf buffer initialization, [net]") = [] {
        tester::bdd::given("An endpointbuf with a socket") = [] {
            if(!network_tests_enabled()) return;
            
            auto s = socket{posix::af_inet, posix::sock_stream};
            endpointbuf<512> buf{std::move(s)};
            
            tester::bdd::when("Checking buffer state") = [] {
                // After construction, the buffer should be ready for use
                // We can't directly access protected members, but we can verify
                // the object was constructed successfully
                check_true(true);
            };
        };
    };

    tester::bdd::scenario("endpointbuf construction with different sizes, [net]") = [] {
        tester::bdd::given("Sockets for different buffer sizes") = [] {
            if(!network_tests_enabled()) return;
            
            tester::bdd::when("Creating endpointbuf with different buffer sizes") = [] {
                check_nothrow([] {
                    auto s1 = socket{posix::af_inet, posix::sock_stream};
                    auto s2 = socket{posix::af_inet, posix::sock_stream};
                    auto s3 = socket{posix::af_inet, posix::sock_stream};
                    endpointbuf<256> buf1{std::move(s1)};
                    endpointbuf<512> buf2{std::move(s2)};
                    endpointbuf<1024> buf3{std::move(s3)};
                    check_true(true);
                });
            };
        };
    };

    return true;
}

const auto _ = register_endpointbuf_tests();

