#include <gtest/gtest.h>
#include "std/extension.hpp"

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std::string_literals;

TEST(StdExtension,Durations)
{
    ASSERT_EQ(24, duration_cast<hours>(days{1}).count());
    ASSERT_EQ(30, duration_cast<days>(months{1}).count());
    ASSERT_EQ(12, duration_cast<months>(years{1}).count());
    ASSERT_EQ(365, duration_cast<days>(years{1}).count());
}

TEST(StdExtension,Durations2Stream)
{
    auto sd = stringstream{};
    sd << days{7};
    ASSERT_EQ("7"s, sd.str());

    auto sm = stringstream{};
    sm << months{40};
    ASSERT_EQ("40"s, sm.str());

    auto sy = stringstream{};
    sy << months{33};
    ASSERT_EQ("33"s, sy.str());
}

TEST(StdExtension,Months2String)
{
    ASSERT_EQ("Jan"s, std::to_string(months{1}));
    ASSERT_EQ("Feb"s, std::to_string(months{2}));
    ASSERT_EQ("Mar"s, std::to_string(months{3}));
    ASSERT_EQ("Apr"s, std::to_string(months{4}));
    ASSERT_EQ("May"s, std::to_string(months{5}));
    ASSERT_EQ("Jun"s, std::to_string(months{6}));
    ASSERT_EQ("Jul"s, std::to_string(months{7}));
    ASSERT_EQ("Aug"s, std::to_string(months{8}));
    ASSERT_EQ("Sep"s, std::to_string(months{9}));
    ASSERT_EQ("Oct"s, std::to_string(months{10}));
    ASSERT_EQ("Nov"s, std::to_string(months{11}));
    ASSERT_EQ("Dec"s, std::to_string(months{12}));
}

TEST(StdExtension,Days2String)
{
    ASSERT_EQ("Wed"s, std::to_string(days{-1}));
    ASSERT_EQ("Thu"s, std::to_string(days{0}));
    ASSERT_EQ("Fri"s, std::to_string(days{1}));
    ASSERT_EQ("Sat"s, std::to_string(days{2}));
    ASSERT_EQ("Sun"s, std::to_string(days{3}));
    ASSERT_EQ("Mon"s, std::to_string(days{4}));
    ASSERT_EQ("Tue"s, std::to_string(days{5}));
    ASSERT_EQ("Wed"s, std::to_string(days{6}));
    ASSERT_EQ("Thu"s, std::to_string(days{7}));
    ASSERT_EQ("Fri"s, std::to_string(days{8}));
    ASSERT_EQ("Sat"s, std::to_string(days{9}));
    ASSERT_EQ("Sun"s, std::to_string(days{10}));
    ASSERT_EQ("Mon"s, std::to_string(days{11}));
    ASSERT_EQ("Tue"s, std::to_string(days{12}));
    ASSERT_EQ("Wed"s, std::to_string(days{13}));
}

TEST(StdExtension,TimePoint2RFC1123)
{
    ASSERT_EQ("Thu, 01 Jan 1970 00:00:00 GMT"s, std::to_rfc1123(system_clock::time_point{0us}));
    ASSERT_EQ("Thu, 01 Jan 1970 00:00:00 GMT"s, std::to_rfc1123(system_clock::time_point{1ms}));
    ASSERT_EQ("Thu, 01 Jan 1970 00:00:01 GMT"s, std::to_rfc1123(system_clock::time_point{1s}));
    ASSERT_EQ("Thu, 01 Jan 1970 00:01:00 GMT"s, std::to_rfc1123(system_clock::time_point{1min}));
    ASSERT_EQ("Thu, 01 Jan 1970 01:00:00 GMT"s, std::to_rfc1123(system_clock::time_point{1h}));
    ASSERT_EQ("Thu, 01 Jan 1970 12:00:00 GMT"s, std::to_rfc1123(system_clock::time_point{12h}));
}

TEST(StdExtension,TimePoint2ISO8601)
{
    ASSERT_EQ("1970-01-01T00:00:00.000Z"s, std::to_iso8601(system_clock::time_point{0us}));
    ASSERT_EQ("1970-01-01T00:00:00.001Z"s, std::to_iso8601(system_clock::time_point{1ms}));
    ASSERT_EQ("1970-01-01T00:00:01.000Z"s, std::to_iso8601(system_clock::time_point{1s}));
    ASSERT_EQ("1970-01-01T00:01:00.000Z"s, std::to_iso8601(system_clock::time_point{1min}));
    ASSERT_EQ("1970-01-01T01:00:00.000Z"s, std::to_iso8601(system_clock::time_point{1h}));
    ASSERT_EQ("1970-01-01T12:00:00.000Z"s, std::to_iso8601(system_clock::time_point{12h}));
}

TEST(StdExtension,TimePoint2String)
{
    ASSERT_EQ("1970-01-01T00:00:00.000Z"s, std::to_string(system_clock::time_point{0us}));
    ASSERT_EQ("1970-01-01T00:00:00.001Z"s, std::to_string(system_clock::time_point{1ms}));
    ASSERT_EQ("1970-01-01T00:00:01.000Z"s, std::to_string(system_clock::time_point{1s}));
    ASSERT_EQ("1970-01-01T00:01:00.000Z"s, std::to_string(system_clock::time_point{1min}));
    ASSERT_EQ("1970-01-01T01:00:00.000Z"s, std::to_string(system_clock::time_point{1h}));
    ASSERT_EQ("1970-01-01T12:00:00.000Z"s, std::to_string(system_clock::time_point{12h}));
}

TEST(StdExtension,String2TimePoint)
{
    ASSERT_EQ(system_clock::time_point{0us}, std::stotp("1970-01-01T00:00:00.000Z"s));
    ASSERT_EQ(system_clock::time_point{1ms}, std::stotp("1970-01-01T00:00:00.001Z"s));
    ASSERT_EQ(system_clock::time_point{1s}, std::stotp("1970-01-01T00:00:01.000Z"s));
    ASSERT_EQ(system_clock::time_point{1min}, std::stotp("1970-01-01T00:01:00.000Z"s));
    ASSERT_EQ(system_clock::time_point{1h}, std::stotp("1970-01-01T01:00:00.000Z"s));
    ASSERT_EQ(system_clock::time_point{12h}, std::stotp("1970-01-01T12:00:00.000Z"s));
}

TEST(StdExtension,Boolean2String)
{
    ASSERT_EQ("true"s, std::to_string(true));
    ASSERT_EQ("false"s, std::to_string(false));
}

TEST(StdExtension,String2Boolean)
{
    ASSERT_EQ(true, std::stob("true"s));
    ASSERT_EQ(true, std::stob("1"s));
    ASSERT_EQ(false, std::stob("false"s));
    ASSERT_EQ(false, std::stob("0"s));
}

TEST(StdExtension,Nullptr2String)
{
    ASSERT_EQ("null"s, std::to_string(nullptr));
}

TEST(StdExtension,String2String)
{
    ASSERT_EQ("abcdefg1234567"s, std::to_string("abcdefg1234567"s));
}
