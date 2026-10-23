// Copyright (c) 2025-2026 Kaius Ruokonen. All rights reserved.
// SPDX-License-Identifier: MIT
// See the LICENSE file in the project root for full license text.

module net;
import tester;
import std;

using namespace net;

namespace {
using namespace std::string_literals;
using namespace std::string_view_literals;
using tester::basic::test_case;
using tester::basic::section;
using tester::assertions::check_eq;
using tester::assertions::check_false;
using tester::assertions::check_true;
}

auto register_header_access_tests()
{
    test_case("headers try_get and value_or, [net][http-headers]") = [] {
        http::headers hs;
        hs.set("Authorization"sv, "Basic Zm9vOmJhcg=="sv);

        check_true(hs.try_get("authorization"sv).has_value());
        check_eq(std::string{*hs.try_get("AUTHORIZATION"sv)}, "Basic Zm9vOmJhcg=="s);
        check_false(hs.try_get("origin"sv).has_value());
        check_eq(hs.value_or("origin"sv, ""s), ""s);
        check_eq(hs.value_or("authorization"sv, ""s), "Basic Zm9vOmJhcg=="s);

        // set owns copies — string and string_view callers both work
        hs.set("X-Trace"s, "abc"s);
        check_eq(std::string{*hs.try_get("x-trace"sv)}, "abc"s);
    };

    return true;
}

auto register_malformed_headers_tests()
{
    test_case("Malformed HTTP headers parsing, [net]") = [] {
        section("Parsing valid header lines") = [] {
            auto raw = "Valid-Header: correct\r\n"
                       "Multiple: Colons: Here\r\n"  // Multiple colons
                       "Name-Only:\r\n"              // Missing value
                       "Extra-Spaces:   value  \r\n" // Extra spaces
                       "\r\n"s;

            auto is = std::istringstream{raw};
            http::headers hs;
            is >> hs;

            check_eq(hs["valid-header"], "correct");
            check_eq(hs["multiple"], "Colons: Here");
            check_eq(hs["name-only"], "");
            check_eq(hs["extra-spaces"], "value");
        };

        auto rejected = [](std::string raw, std::string_view reason) {
            auto is = std::istringstream{std::move(raw)};
            http::headers hs;
            auto threw = false;
            try
            {
                is >> hs;
            }
            catch(const std::runtime_error& e)
            {
                threw = std::string_view{e.what()}.contains(reason);
            }
            check_true(threw);
        };

        section("A header line with no colon is rejected") = [rejected] {
            rejected("Host: example\r\nNo-Colon-Line\r\n\r\n"s, "malformed header line");
        };

        section("Obsolete line folding is rejected") = [rejected] {
            rejected("Content-Length: 5\r\n 0\r\n\r\n"s, "obsolete line folding");
            rejected("Host: example\r\n\tfolded\r\n\r\n"s, "obsolete line folding");
        };

        section("An empty header name is rejected") = [rejected] {
            rejected(":Value-Only\r\n\r\n"s, "invalid header name");
        };

        // RFC 9112 §5.1: a proxy that ignores these may disagree on the body length.
        section("Whitespace or controls in the name are rejected") = [rejected] {
            rejected("Content-Length : 5\r\n\r\n"s, "invalid header name");
            rejected("Content-Length\t: 5\r\n\r\n"s, "invalid header name");
            rejected("\vContent-Length: 5\r\n\r\n"s, "invalid header name");
            rejected("Content-Length\v: 5\r\n\r\n"s, "invalid header name");
            rejected("Content\0Length: 5\r\n\r\n"s, "invalid header name");
            rejected("X(y): 1\r\n\r\n"s, "invalid header name");
        };

        section("Controls in the value are rejected") = [rejected] {
            rejected("X: a\rb\r\n\r\n"s, "invalid header value");
            rejected("X: a\0b\r\n\r\n"s, "invalid header value");
            rejected("X: a\x7f\r\n\r\n"s, "invalid header value");
            rejected("Content-Length: 5\v\r\n\r\n"s, "invalid header value");
        };

        section("HTAB and obs-text stay valid in values") = [] {
            auto is = std::istringstream{"X: a\tb\r\nY: \xc3\xa4\r\nZ:\t trimmed \t\r\n\r\n"s};
            http::headers hs;
            is >> hs;
            check_eq(hs["x"], "a\tb");
            check_eq(hs["y"], "\xc3\xa4");
            check_eq(hs["z"], "trimmed");
        };

        // `\r\r\n` used to end the fields here while read_request_head kept
        // reading, so the Transfer-Encoding after it escaped the TE check.
        section("A CR that does not end the line is not the end of the fields") = [rejected] {
            rejected("Host: a\r\n\r\r\nTransfer-Encoding: chunked\r\n\r\n"s, "malformed header line");
        };

        section("The final line ending is left for net::crlf") = [] {
            auto is = std::istringstream{"Host: a\r\n\r\nbody"s};
            http::headers hs;
            is >> hs >> net::crlf;
            check_eq(hs["host"], "a");
            auto rest = ""s;
            std::getline(is, rest);
            check_eq(rest, "body");
        };

        section("Line ending with LF only") = [] {
            auto raw = "Header-LF: value\n\n"s;
            auto is = std::istringstream{raw};
            http::headers hs;
            is >> hs;
            check_eq(hs["header-lf"], "value");
        };

        section("Mixed line endings (CRLF and LF)") = [] {
            auto raw = "Header1: v1\r\nHeader2: v2\n\n"s;
            auto is = std::istringstream{raw};
            http::headers hs;
            is >> hs;
            check_eq(hs["header1"], "v1");
            check_eq(hs["header2"], "v2");
        };

        // Regression: last-wins duplicate Content-Length enabled request
        // smuggling when a front-end proxy used the first value.
        section("Duplicate Content-Length is rejected") = [] {
            auto raw = "Content-Length: 4\r\n"
                       "Content-Length: 0\r\n"
                       "\r\n"s;
            auto is = std::istringstream{raw};
            http::headers hs;
            auto threw = false;
            try
            {
                is >> hs;
            }
            catch(const std::runtime_error& e)
            {
                threw = std::string_view{e.what()}.contains("duplicate Content-Length");
            }
            check_true(threw);
        };

        // Regression: last-wins duplicate Host enabled authority poisoning when
        // a front-end proxy routed on the first Host and the app used the last.
        section("Duplicate Host is rejected") = [] {
            auto raw = "Host: victim.example\r\n"
                       "Host: evil.attacker\r\n"
                       "\r\n"s;
            auto is = std::istringstream{raw};
            http::headers hs;
            auto threw = false;
            try
            {
                is >> hs;
            }
            catch(const std::runtime_error& e)
            {
                threw = std::string_view{e.what()}.contains("duplicate Host");
            }
            check_true(threw);
        };
    };

    return true;
}

const auto _access = register_header_access_tests();
const auto _ = register_malformed_headers_tests();
