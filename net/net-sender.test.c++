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

auto register_sender_tests()
{
    if(!network_tests_enabled()) return false;

    tester::bdd::scenario("Basic construction, [net]") = [] {
        tester::bdd::given("A sender for group 228.0.0.4 and service test") = [] {
            auto sder = net::sender{"228.0.0.4","test"};
            check_eq(sder.group(),"228.0.0.4");
            check_eq(sder.service_or_port(),"test");
        };
    };

tester::bdd::scenario("Distribute multicast, [net]") = [] {
        tester::bdd::given("Distributing to group 228.0.0.4 on port 54321") = [] {
            check_nothrow([] {
                auto s = net::distribute("228.0.0.4", "54321", 3);
                check_true(!(!s));
            });
        };
    };

tester::bdd::scenario("UDP Distribute, [net]") = [] {
        tester::bdd::given("Distributing to localhost:syslog") = [] {
            check_nothrow([] {
                auto s = net::distribute("localhost", "syslog");
                check_true(!(!s));
            });
        };
    };

    return true;
}

const auto _ = register_sender_tests();
