module net;
import tester;
import std;

namespace {
using tester::basic::test_case;
using tester::assertions::check_eq;
using tester::assertions::check_true;
}

auto extension_test_reg = test_case("Std Extension") = [] {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    tester::bdd::scenario("Months to String") = [] {
        using namespace std::chrono;
        check_eq("Jan", ext::month_to_string(month{1}));
        check_eq("Feb", ext::month_to_string(month{2}));
        check_eq("Mar", ext::month_to_string(month{3}));
        check_eq("Apr", ext::month_to_string(month{4}));
        check_eq("May", ext::month_to_string(month{5}));
        check_eq("Jun", ext::month_to_string(month{6}));
        check_eq("Jul", ext::month_to_string(month{7}));
        check_eq("Aug", ext::month_to_string(month{8}));
        check_eq("Sep", ext::month_to_string(month{9}));
        check_eq("Oct", ext::month_to_string(month{10}));
        check_eq("Nov", ext::month_to_string(month{11}));
        check_eq("Dec", ext::month_to_string(month{12}));
    };

    tester::bdd::scenario("Weekday to String") = [] {
        using namespace std::chrono;
        check_eq("Sun", ext::weekday_to_string(weekday{0}));
        check_eq("Mon", ext::weekday_to_string(weekday{1}));
        check_eq("Tue", ext::weekday_to_string(weekday{2}));
        check_eq("Wed", ext::weekday_to_string(weekday{3}));
        check_eq("Thu", ext::weekday_to_string(weekday{4}));
        check_eq("Fri", ext::weekday_to_string(weekday{5}));
        check_eq("Sat", ext::weekday_to_string(weekday{6}));
    };

    tester::bdd::scenario("Time Point to RFC1123") = [] {
        using namespace std::chrono;
        using namespace std::chrono_literals;
        check_eq("Thu, 01 Jan 1970 00:00:00 GMT", ext::to_rfc1123(system_clock::time_point{0us}));
        check_eq("Thu, 01 Jan 1970 00:00:00 GMT", ext::to_rfc1123(system_clock::time_point{1ms}));
        check_eq("Thu, 01 Jan 1970 00:00:01 GMT", ext::to_rfc1123(system_clock::time_point{1s}));
        check_eq("Thu, 01 Jan 1970 00:01:00 GMT", ext::to_rfc1123(system_clock::time_point{1min}));
        check_eq("Thu, 01 Jan 1970 01:00:00 GMT", ext::to_rfc1123(system_clock::time_point{1h}));
        check_eq("Thu, 01 Jan 1970 12:00:00 GMT", ext::to_rfc1123(system_clock::time_point{12h}));
    };
#pragma clang diagnostic pop

#if defined(__APPLE__) && defined(__clang__) && (__clang_major__ >= 22)
    tester::bdd::scenario("std::vformat (chrono) RFC1123") = [] {
        // clang-22 (macOS) has a frontend crash compiling chrono formatting via std::format/vformat
        // in our modules setup. Keep CI coverage (Linux/clang-20) while avoiding local crashes.
        check_true(true);
    };
#else
    tester::bdd::scenario("std::vformat (chrono) RFC1123") = [] {
        using namespace std::chrono;
        using namespace std::chrono_literals;

        const auto fmt_rfc1123 = [](system_clock::time_point tp) {
            const auto t = floor<seconds>(tp);
            return std::vformat("{:%a, %d %b %Y %H:%M:%S GMT}", std::make_format_args(t));
        };

        check_eq("Thu, 01 Jan 1970 00:00:00 GMT", fmt_rfc1123(system_clock::time_point{0us}));
        check_eq("Thu, 01 Jan 1970 00:00:00 GMT", fmt_rfc1123(system_clock::time_point{1ms}));
        check_eq("Thu, 01 Jan 1970 00:00:01 GMT", fmt_rfc1123(system_clock::time_point{1s}));
        check_eq("Thu, 01 Jan 1970 00:01:00 GMT", fmt_rfc1123(system_clock::time_point{1min}));
        check_eq("Thu, 01 Jan 1970 01:00:00 GMT", fmt_rfc1123(system_clock::time_point{1h}));
        check_eq("Thu, 01 Jan 1970 12:00:00 GMT", fmt_rfc1123(system_clock::time_point{12h}));
    };
#endif

    tester::bdd::scenario("String to Time Point") = [] {
        using namespace std::chrono;
        using namespace std::chrono_literals;
        check_eq(system_clock::time_point{0us}, ext::to_time_point("1970-01-01T00:00:00.000Z"));
        check_eq(system_clock::time_point{1ms}, ext::to_time_point("1970-01-01T00:00:00.001Z"));
        check_eq(system_clock::time_point{1s}, ext::to_time_point("1970-01-01T00:00:01.000Z"));
        check_eq(system_clock::time_point{1min}, ext::to_time_point("1970-01-01T00:01:00.000Z"));
        check_eq(system_clock::time_point{1h}, ext::to_time_point("1970-01-01T01:00:00.000Z"));
        check_eq(system_clock::time_point{12h}, ext::to_time_point("1970-01-01T12:00:00.000Z"));
        check_eq(system_clock::time_point{days{1}}, ext::to_time_point("1970-01-02T00:00:00.000Z"));
    };
};

