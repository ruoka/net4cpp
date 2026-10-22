// Copyright (c) 2025-2026 Kaius Ruokonen. All rights reserved.
// SPDX-License-Identifier: MIT
// See the LICENSE file in the project root for full license text.

module net;
import tester;
import std;

using namespace net;

namespace {
using namespace std::string_literals;
using tester::basic::test_case;
using tester::basic::section;
using tester::assertions::check_eq;
using tester::assertions::check_false;
using tester::assertions::check_true;
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
            rejected(":Value-Only\r\n\r\n"s, "malformed header line");
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

const auto _ = register_malformed_headers_tests();
