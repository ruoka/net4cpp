module net;
import :address_info;
import :posix;
import tester;
import std;


using namespace net;

namespace {
using tester::assertions::check_true;
using tester::assertions::check_false;
using tester::assertions::check_eq;
using namespace std::string_view_literals;
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

    return true;
}

const auto _ = register_address_info_tests();
