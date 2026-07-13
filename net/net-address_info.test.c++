module net;
import tester;
import :posix;
import std;


using namespace net;

namespace {
using tester::basic::test_case;
using tester::assertions::check_true;
using tester::assertions::check_eq;
}

auto register_address_info_tests()
{
    tester::bdd::scenario("Stream construction, [net]") = [] {
        tester::bdd::given("A localhost stream address") = [] {
            auto ai = address_info{"localhost", "54321", posix::sock_stream};
            auto b = begin(ai);
            auto e = end(ai);
            check_true(b != e);
            for(const auto& a : ai)
                check_eq(a.ai_socktype, posix::sock_stream);
        };
    };

    tester::bdd::scenario("Datagram construction, [net]") = [] {
        tester::bdd::given("A localhost datagram address") = [] {
            auto ai = address_info{"localhost", "54321", posix::sock_dgram};
            auto b = begin(ai);
            auto e = end(ai);
            check_true(b != e);
            for(const auto& a : ai)
                check_eq(a.ai_socktype, posix::sock_dgram);
        };
    };

    tester::bdd::scenario("No host construction, [net]") = [] {
        tester::bdd::given("An empty host stream address") = [] {
            auto ai = address_info{"", "54321", posix::sock_stream};
            auto b = begin(ai);
            auto e = end(ai);
            check_true(b != e);
            for(const auto& a : ai)
                check_eq(a.ai_socktype, posix::sock_stream);
        };
    };

    return true;
}

const auto _ = register_address_info_tests();
