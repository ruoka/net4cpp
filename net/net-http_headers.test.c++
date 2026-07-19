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
        section("Parsing broken header lines") = [] {
            auto raw = "Valid-Header: correct\r\n"
                       "No-Colon-Line\r\n"           // Missing colon
                       "Multiple: Colons: Here\r\n"  // Multiple colons
                       " Empty-Name: value\r\n"      // Leading space in name
                       "Name-Only:\r\n"              // Missing value
                       ":Value-Only\r\n"             // Missing name
                       "Extra-Spaces:   value  \r\n" // Extra spaces
                       "\r\n"s;
            
            auto is = std::istringstream{raw};
            http::headers hs;
            is >> hs;

            check_eq(hs["valid-header"], "correct");
            check_false(hs.contains("no-colon-line"));
            check_eq(hs["multiple"], "Colons: Here");
            check_eq(hs["empty-name"], "value");
            check_eq(hs["name-only"], "");
            check_eq(hs[""], "Value-Only");
            check_eq(hs["extra-spaces"], "value");
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
    };

    return true;
}

const auto _ = register_malformed_headers_tests();
