module net;
import tester;
import std;

using namespace net;

namespace {
using namespace std::string_literals;
using tester::basic::test_case;
using tester::assertions::check_eq;
using tester::assertions::check_false;
}

auto register_malformed_headers_tests()
{
    // 1. Parsing broken header lines
    {
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
    }

    // 2. Line ending variations
    {
        auto raw = "Header-LF: value\n\n"s;
        auto is = std::istringstream{raw};
        http::headers hs;
        is >> hs;
        check_eq(hs["header-lf"], "value");
    }

    {
        auto raw = "Header1: v1\r\nHeader2: v2\n\n"s;
        auto is = std::istringstream{raw};
        http::headers hs;
        is >> hs;
        check_eq(hs["header1"], "v1");
        check_eq(hs["header2"], "v2");
    }

    return true;
}

const auto _ = register_malformed_headers_tests();
