module;
#include <sys/socket.h>
#include <netdb.h>

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

auto connector_test_reg = test_case("Connector") = [] {
    if(!network_tests_enabled()) return;
    tester::bdd::scenario("Basic construction") = [] {
        tester::bdd::given("A connector to google.com:http") = [] {
            const auto ctor = net::connector{"google.com","http"};
            check_eq(ctor.host(), "google.com");
            check_eq(ctor.service_or_port(), "http");
            check_eq(ctor.timeout(), default_connect_timeout);
        };
    };

    tester::bdd::scenario("Connecting to a host") = [] {
        tester::bdd::given("A connection to google.com:http") = [] {
            check_nothrow([] {
                const auto s = net::connect("google.com","http");
                check_true(!(!s));
            });
        };
    };

    tester::bdd::scenario("Connection timeout and failures") = [] {
        tester::bdd::given("A connector to localhost on a busy port") = [] {
            const auto address = address_info{"localhost", "1999", SOCK_STREAM, AI_PASSIVE};
            net::socket s{address->ai_family, address->ai_socktype, address->ai_protocol};
            posix::bind(s, address->ai_addr, address->ai_addrlen);
            auto ctor = net::connector{"localhost", "1999"};
            using namespace std::chrono_literals;
            ctor.timeout(100ms);
            // We don't necessarily want to assert a throw here as it depends on system state
        };
    };
};
