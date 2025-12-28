module net;
import tester;
import std;

using namespace net;

namespace {
using tester::basic::test_case;
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

auto sender_test_reg = test_case("Sender") = [] {
    if(!network_tests_enabled()) return;
    tester::bdd::scenario("Basic construction") = [] {
        tester::bdd::given("A sender for group 228.0.0.4 and service test") = [] {
            auto sder = net::sender{"228.0.0.4","test"};
            check_eq(sder.group(),"228.0.0.4");
            check_eq(sder.service_or_port(),"test");
        };
    };

    tester::bdd::scenario("Distribute multicast") = [] {
        tester::bdd::given("Distributing to group 228.0.0.4 on port 54321") = [] {
            check_nothrow([] {
                auto s = net::distribute("228.0.0.4", "54321", 3);
                check_true(!(!s));
            });
        };
    };

    tester::bdd::scenario("UDP Distribute") = [] {
        tester::bdd::given("Distributing to localhost:syslog") = [] {
            check_nothrow([] {
                auto s = net::distribute("localhost", "syslog");
                check_true(!(!s));
            });
        };
    };
};
