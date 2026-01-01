module net;
import tester;
import std;

namespace {
using tester::assertions::check_eq;
using tester::assertions::check_true;
using tester::assertions::check_throws_as;
using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace std::chrono;
}

auto register_utils_tests()
{
    tester::bdd::scenario("String to long long conversion, [net]") = [] {
        tester::bdd::given("Valid number strings") = [] {
            check_eq(0ll, utils::stoll("0"));
            check_eq(42ll, utils::stoll("42"));
            check_eq(-42ll, utils::stoll("-42"));
            check_eq(1234567890ll, utils::stoll("1234567890"));
            check_eq(-1234567890ll, utils::stoll("-1234567890"));
        };

        tester::bdd::given("Invalid number strings") = [] {
            tester::bdd::when("String contains non-numeric characters") = [] {
                tester::bdd::then("Throws std::invalid_argument") = [] {
                    check_throws_as([] { utils::stoll("abc"); }, std::invalid_argument{""});
                    check_throws_as([] { utils::stoll("12abc"); }, std::invalid_argument{""});
                    check_throws_as([] { utils::stoll("abc12"); }, std::invalid_argument{""});
                };
            };

            tester::bdd::when("String is empty") = [] {
                tester::bdd::then("Throws std::invalid_argument") = [] {
                    check_throws_as([] { utils::stoll(""); }, std::invalid_argument{""});
                };
            };

            tester::bdd::when("String is out of range") = [] {
                tester::bdd::then("Throws std::invalid_argument") = [] {
                    check_throws_as([] { utils::stoll("99999999999999999999"); }, std::invalid_argument{""});
                };
            };
        };
    };

    tester::bdd::scenario("String view trim functions, [net]") = [] {
        tester::bdd::given("Strings with leading/trailing whitespace") = [] {
            tester::bdd::when("Using trim_start") = [] {
                tester::bdd::then("Removes leading whitespace") = [] {
                    check_eq("hello"sv, utils::trim_start("  hello"sv));
                    check_eq("hello"sv, utils::trim_start("\t\nhello"sv));
                    check_eq("hello  "sv, utils::trim_start("  hello  "sv));
                    check_eq(""sv, utils::trim_start("   "sv));
                    check_eq("hello"sv, utils::trim_start("hello"sv));
                };
            };

            tester::bdd::when("Using trim_end") = [] {
                tester::bdd::then("Removes trailing whitespace") = [] {
                    check_eq("hello"sv, utils::trim_end("hello  "sv));
                    check_eq("hello"sv, utils::trim_end("hello\t\n"sv));
                    check_eq("  hello"sv, utils::trim_end("  hello  "sv));
                    check_eq(""sv, utils::trim_end("   "sv));
                    check_eq("hello"sv, utils::trim_end("hello"sv));
                };
            };

            tester::bdd::when("Using trim") = [] {
                tester::bdd::then("Removes both leading and trailing whitespace") = [] {
                    check_eq("hello"sv, utils::trim("  hello  "sv));
                    check_eq("hello"sv, utils::trim("\t\nhello\t\n"sv));
                    check_eq(""sv, utils::trim("   "sv));
                    check_eq("hello"sv, utils::trim("hello"sv));
                };
            };
        };
    };

    tester::bdd::scenario("String view trim to string conversion, [net]") = [] {
        tester::bdd::given("String views with whitespace") = [] {
            tester::bdd::when("Using trim_start_to_string") = [] {
                tester::bdd::then("Returns std::string with leading whitespace removed") = [] {
                    check_eq("hello"s, utils::trim_start_to_string("  hello"sv));
                    check_eq("hello  "s, utils::trim_start_to_string("  hello  "sv));
                };
            };

            tester::bdd::when("Using trim_end_to_string") = [] {
                tester::bdd::then("Returns std::string with trailing whitespace removed") = [] {
                    check_eq("hello"s, utils::trim_end_to_string("hello  "sv));
                    check_eq("  hello"s, utils::trim_end_to_string("  hello  "sv));
                };
            };
        };
    };

    tester::bdd::scenario("Trim view function, [net]") = [] {
        tester::bdd::given("String views with whitespace") = [] {
            tester::bdd::when("Using trim_view") = [] {
                tester::bdd::then("Returns string_view with whitespace removed") = [] {
                    check_eq("hello"sv, utils::trim_view("  hello  "sv));
                    check_eq("hello"sv, utils::trim_view("\t\nhello\t\n"sv));
                    check_eq(""sv, utils::trim_view("   "sv));
                    check_eq("hello"sv, utils::trim_view("hello"sv));
                };
            };
        };
    };

    tester::bdd::scenario("RFC 1123 date parsing, [net]") = [] {
        tester::bdd::given("Valid RFC 1123 date strings") = [] {
            tester::bdd::when("Parsing dates with different weekdays") = [] {
                tester::bdd::then("Returns correct time_point") = [] {
                    // Test all weekdays
                    auto date1 = utils::parse_rfc1123_date("Sun, 01 Jan 2026 00:00:00 GMT"sv);
                    auto date2 = utils::parse_rfc1123_date("Mon, 01 Jan 2026 00:00:00 GMT"sv);
                    auto date3 = utils::parse_rfc1123_date("Tue, 01 Jan 2026 00:00:00 GMT"sv);
                    auto date4 = utils::parse_rfc1123_date("Wed, 01 Jan 2026 00:00:00 GMT"sv);
                    auto date5 = utils::parse_rfc1123_date("Thu, 01 Jan 2026 00:00:00 GMT"sv);
                    auto date6 = utils::parse_rfc1123_date("Fri, 01 Jan 2026 00:00:00 GMT"sv);
                    auto date7 = utils::parse_rfc1123_date("Sat, 01 Jan 2026 00:00:00 GMT"sv);
                    
                    // All should parse to the same date (weekday is ignored)
                    check_eq(date1, date2);
                    check_eq(date2, date3);
                    check_eq(date3, date4);
                    check_eq(date4, date5);
                    check_eq(date5, date6);
                    check_eq(date6, date7);
                };
            };

            tester::bdd::when("Parsing dates with all months") = [] {
                tester::bdd::then("Returns correct time_point for each month") = [] {
                    check_eq(sys_days{year{2026}/month{1}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 Jan 2026 00:00:00 GMT"sv));
                    check_eq(sys_days{year{2026}/month{2}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 Feb 2026 00:00:00 GMT"sv));
                    check_eq(sys_days{year{2026}/month{3}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 Mar 2026 00:00:00 GMT"sv));
                    check_eq(sys_days{year{2026}/month{4}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 Apr 2026 00:00:00 GMT"sv));
                    check_eq(sys_days{year{2026}/month{5}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 May 2026 00:00:00 GMT"sv));
                    check_eq(sys_days{year{2026}/month{6}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 Jun 2026 00:00:00 GMT"sv));
                    check_eq(sys_days{year{2026}/month{7}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 Jul 2026 00:00:00 GMT"sv));
                    check_eq(sys_days{year{2026}/month{8}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 Aug 2026 00:00:00 GMT"sv));
                    check_eq(sys_days{year{2026}/month{9}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 Sep 2026 00:00:00 GMT"sv));
                    check_eq(sys_days{year{2026}/month{10}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 Oct 2026 00:00:00 GMT"sv));
                    check_eq(sys_days{year{2026}/month{11}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 Nov 2026 00:00:00 GMT"sv));
                    check_eq(sys_days{year{2026}/month{12}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 Dec 2026 00:00:00 GMT"sv));
                };
            };

            tester::bdd::when("Parsing dates with various times") = [] {
                tester::bdd::then("Returns correct time_point") = [] {
                    check_eq(sys_days{year{2026}/month{1}/day{1}} + hours{12} + minutes{34} + seconds{56},
                             utils::parse_rfc1123_date("Thu, 01 Jan 2026 12:34:56 GMT"sv));
                    check_eq(sys_days{year{2026}/month{1}/day{1}} + hours{23} + minutes{59} + seconds{59},
                             utils::parse_rfc1123_date("Thu, 01 Jan 2026 23:59:59 GMT"sv));
                    check_eq(sys_days{year{2026}/month{1}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 Jan 2026 00:00:00 GMT"sv));
                };
            };

            tester::bdd::when("Parsing dates with different years") = [] {
                tester::bdd::then("Returns correct time_point") = [] {
                    check_eq(sys_days{year{1970}/month{1}/day{1}} + hours{0} + minutes{0} + seconds{0},
                             utils::parse_rfc1123_date("Thu, 01 Jan 1970 00:00:00 GMT"sv));
                    check_eq(sys_days{year{2099}/month{12}/day{31}} + hours{23} + minutes{59} + seconds{59},
                             utils::parse_rfc1123_date("Tue, 31 Dec 2099 23:59:59 GMT"sv));
                };
            };
        };

        tester::bdd::given("Invalid RFC 1123 date strings") = [] {
            tester::bdd::when("String is missing comma") = [] {
                tester::bdd::then("Throws std::invalid_argument") = [] {
                    check_throws_as([] { utils::parse_rfc1123_date("Thu 01 Jan 2026 00:00:00 GMT"sv); },
                                    std::invalid_argument{""});
                };
            };

            tester::bdd::when("String has invalid month") = [] {
                tester::bdd::then("Throws std::invalid_argument") = [] {
                    check_throws_as([] { utils::parse_rfc1123_date("Thu, 01 Xxx 2026 00:00:00 GMT"sv); },
                                    std::invalid_argument{""});
                    check_throws_as([] { utils::parse_rfc1123_date("Thu, 01 Invalid 2026 00:00:00 GMT"sv); },
                                    std::invalid_argument{""});
                };
            };

            tester::bdd::when("String has invalid day format") = [] {
                tester::bdd::then("Throws std::invalid_argument") = [] {
                    check_throws_as([] { utils::parse_rfc1123_date("Thu, XX Jan 2026 00:00:00 GMT"sv); },
                                    std::invalid_argument{""});
                };
            };

            tester::bdd::when("String has invalid year format") = [] {
                tester::bdd::then("Throws std::invalid_argument") = [] {
                    check_throws_as([] { utils::parse_rfc1123_date("Thu, 01 Jan XXXX 00:00:00 GMT"sv); },
                                    std::invalid_argument{""});
                    check_throws_as([] { utils::parse_rfc1123_date("Thu, 01 Jan 26 00:00:00 GMT"sv); },
                                    std::invalid_argument{""});
                };
            };

            tester::bdd::when("String has invalid time format") = [] {
                tester::bdd::then("Throws std::invalid_argument") = [] {
                    check_throws_as([] { utils::parse_rfc1123_date("Thu, 01 Jan 2026 XX:00:00 GMT"sv); },
                                    std::invalid_argument{""});
                    check_throws_as([] { utils::parse_rfc1123_date("Thu, 01 Jan 2026 00:XX:00 GMT"sv); },
                                    std::invalid_argument{""});
                    check_throws_as([] { utils::parse_rfc1123_date("Thu, 01 Jan 2026 00:00:XX GMT"sv); },
                                    std::invalid_argument{""});
                    check_throws_as([] { utils::parse_rfc1123_date("Thu, 01 Jan 2026 00-00-00 GMT"sv); },
                                    std::invalid_argument{""});
                };
            };

            tester::bdd::when("String is too short") = [] {
                tester::bdd::then("Throws std::invalid_argument") = [] {
                    check_throws_as([] { utils::parse_rfc1123_date("Thu, 01 Jan"sv); },
                                    std::invalid_argument{""});
                    check_throws_as([] { utils::parse_rfc1123_date("Thu"sv); },
                                    std::invalid_argument{""});
                    check_throws_as([] { utils::parse_rfc1123_date(""sv); },
                                    std::invalid_argument{""});
                };
            };

            tester::bdd::when("String is missing time") = [] {
                tester::bdd::then("Throws std::invalid_argument") = [] {
                    check_throws_as([] { utils::parse_rfc1123_date("Thu, 01 Jan 2026"sv); },
                                    std::invalid_argument{""});
                };
            };
        };
    };

    return true;
}

const auto _ = register_utils_tests();

