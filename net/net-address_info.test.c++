module net;
import :address_info;
import :posix;
import tester;
import std;


using namespace net;

namespace {
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

    // Regression: URI IPv6 literals keep brackets; getaddrinfo rejects "[::1]".
    tester::bdd::scenario("Bracketed IPv6 literal resolves, [net]") = [] {
        auto ai = address_info{"[::1]", "80", posix::sock_stream};
        auto b = begin(ai);
        auto e = end(ai);
        check_true(b != e);
        for(const auto& a : ai)
            check_eq(a.ai_family, posix::af_inet6);
    };

    // Numeric IPv4 literals must keep working without AI_NUMERICHOST.
    tester::bdd::scenario("IPv4 literal resolves without numeric-host heuristic, [net]") = [] {
        auto ai = address_info{"127.0.0.1", "80", posix::sock_stream};
        auto b = begin(ai);
        auto e = end(ai);
        check_true(b != e);
        for(const auto& a : ai)
            check_eq(a.ai_family, posix::af_inet);
    };

    // Regression: a leading digit must not force AI_NUMERICHOST. RFC 1123
    // digit-leading DNS names then fail with EAI_NONAME even when DNS works.
    // Self-gates when the probe name cannot be resolved in this environment.
    tester::bdd::scenario("Digit-leading DNS hostname resolves, [net]") = [] {
        posix::addrinfo hints{};
        hints.ai_family = posix::af_unspec;
        hints.ai_socktype = posix::sock_stream;
        posix::addrinfo* probe = nullptr;
        const auto probe_err = posix::getaddrinfo("9gag.com", "80", &hints, &probe);
        if(probe_err != 0)
            return;
        posix::freeaddrinfo(probe);

        auto ai = address_info{"9gag.com", "80", posix::sock_stream};
        check_true(begin(ai) != end(ai));
    };

    return true;
}

const auto _ = register_address_info_tests();
