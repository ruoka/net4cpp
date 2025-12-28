module;
#include <sys/socket.h>

module net;
import tester;
import std;


using namespace net;

namespace {
using tester::basic::test_case;
using tester::assertions::check_true;
using tester::assertions::check_eq;
}

auto address_info_test_reg = test_case("Address Info construction") = [] {
    tester::bdd::scenario("Stream construction") = [] {
        tester::bdd::given("A localhost stream address") = [] {
            auto ai = address_info{"localhost", "54321", SOCK_STREAM};
            auto b = begin(ai);
            auto e = end(ai);
            check_true(b != e);
            for(const auto& a : ai)
                check_eq(a.ai_socktype, SOCK_STREAM);
        };
    };

    tester::bdd::scenario("Datagram construction") = [] {
        tester::bdd::given("A localhost datagram address") = [] {
            auto ai = address_info{"localhost", "54321", SOCK_DGRAM};
            auto b = begin(ai);
            auto e = end(ai);
            check_true(b != e);
            for(const auto& a : ai)
                check_eq(a.ai_socktype, SOCK_DGRAM);
        };
    };

    tester::bdd::scenario("No host construction") = [] {
        tester::bdd::given("An empty host stream address") = [] {
            auto ai = address_info{"", "54321", SOCK_STREAM};
            auto b = begin(ai);
            auto e = end(ai);
            check_true(b != e);
            for(const auto& a : ai)
                check_eq(a.ai_socktype, SOCK_STREAM);
        };
    };
};
