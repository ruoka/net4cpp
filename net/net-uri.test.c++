module net;
import tester;
import std;

using namespace net;

namespace {
using tester::basic::test_case;
using tester::assertions::check_true;
using tester::assertions::check_false;
using tester::assertions::check_eq;
}

auto register_uri_tests()
{
    tester::bdd::scenario("Full HTTP URI, [net]") = [] {
        tester::bdd::given("A full http uri string") = [] {
            auto raw = "http://kake:passwd@www.appinf.com:88/sample/isin?eq=FI123456789#frags";
            auto parsed = net::uri{raw};

            check_true(static_cast<bool>(parsed.absolute));
            check_eq(static_cast<std::string_view>(parsed.scheme), "http");
            check_eq(static_cast<std::string_view>(parsed.userinfo), "kake:passwd");
            check_eq(static_cast<std::string_view>(parsed.host), "www.appinf.com");
            check_eq(static_cast<std::string_view>(parsed.port), "88");
            check_eq(static_cast<std::string_view>(parsed.path), "/sample/isin");
            check_eq(parsed.path[0], "");
            check_eq(parsed.path[1], "sample");
            check_eq(parsed.path[2], "isin");
            check_eq(static_cast<std::string_view>(parsed.query), "eq=FI123456789");
            check_eq(static_cast<std::string_view>(parsed.fragment), "frags");
        };
    };

    tester::bdd::scenario("Simple HTTPS URI, [net]") = [] {
        tester::bdd::given("A simple https uri string") = [] {
            auto raw = "https://localhost/test/13";
            auto parsed = net::uri{raw};

            check_true(static_cast<bool>(parsed.absolute));
            check_eq(static_cast<std::string_view>(parsed.scheme), "https");
            check_eq(static_cast<std::string_view>(parsed.userinfo), "");
            check_eq(static_cast<std::string_view>(parsed.host), "localhost");
            check_eq(static_cast<std::string_view>(parsed.port), "");
            check_eq(static_cast<std::string_view>(parsed.path), "/test/13");
            check_eq(parsed.path[0], "");
            check_eq(parsed.path[1], "test");
            check_eq(parsed.path[2], "13");
            check_eq(static_cast<std::string_view>(parsed.query), "");
            check_eq(static_cast<std::string_view>(parsed.fragment), "");
        };
    };

    tester::bdd::scenario("Relative URI, [net]") = [] {
        tester::bdd::given("A relative uri string") = [] {
            auto raw = "../../sample/55#frags3";
            auto parsed = net::uri{raw};

            check_false(static_cast<bool>(parsed.absolute));
            check_eq(static_cast<std::string_view>(parsed.scheme), "");
            check_eq(static_cast<std::string_view>(parsed.userinfo), "");
            check_eq(static_cast<std::string_view>(parsed.host), "");
            check_eq(static_cast<std::string_view>(parsed.port), "");
            check_eq(static_cast<std::string_view>(parsed.path), "../../sample/55");
            check_eq(parsed.path[0], "..");
            check_eq(parsed.path[1], "..");
            check_eq(parsed.path[2], "sample");
            check_eq(parsed.path[3], "55");
            check_eq(static_cast<std::string_view>(parsed.query), "");
            check_eq(static_cast<std::string_view>(parsed.fragment), "frags3");
        };
    };

    tester::bdd::scenario("Port without host, [net]") = [] {
        tester::bdd::given("A URI with port but no host") = [] {
            auto raw = "FIXT.1.1://KAKE@:2112/xxx?tail=10";
            auto parsed = net::uri{raw};

            check_true(static_cast<bool>(parsed.absolute));
            check_eq(static_cast<std::string_view>(parsed.scheme), "FIXT.1.1");
            check_eq(static_cast<std::string_view>(parsed.userinfo), "KAKE");
            check_eq(static_cast<std::string_view>(parsed.host), "");
            check_eq(static_cast<std::string_view>(parsed.port), "2112");
            check_eq(static_cast<std::string_view>(parsed.path), "/xxx");
            check_eq(static_cast<std::string_view>(parsed.query), "tail=10");
            check_eq(static_cast<std::string_view>(parsed.fragment), "");
        };
    };

    return true;
}

const auto _ = register_uri_tests();
