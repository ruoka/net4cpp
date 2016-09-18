#include <gtest/gtest.h>
#include "std/extension.hpp"

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace ext;

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
    ASSERT_EQ("Jan"s, to_string(months{1}));
    ASSERT_EQ("Feb"s, to_string(months{2}));
    ASSERT_EQ("Mar"s, to_string(months{3}));
    ASSERT_EQ("Apr"s, to_string(months{4}));
    ASSERT_EQ("May"s, to_string(months{5}));
    ASSERT_EQ("Jun"s, to_string(months{6}));
    ASSERT_EQ("Jul"s, to_string(months{7}));
    ASSERT_EQ("Aug"s, to_string(months{8}));
    ASSERT_EQ("Sep"s, to_string(months{9}));
    ASSERT_EQ("Oct"s, to_string(months{10}));
    ASSERT_EQ("Nov"s, to_string(months{11}));
    ASSERT_EQ("Dec"s, to_string(months{12}));
}

TEST(StdExtension,Days2String)
{
    ASSERT_EQ("Wed"s, to_string(days{-1}));
    ASSERT_EQ("Thu"s, to_string(days{0}));
    ASSERT_EQ("Fri"s, to_string(days{1}));
    ASSERT_EQ("Sat"s, to_string(days{2}));
    ASSERT_EQ("Sun"s, to_string(days{3}));
    ASSERT_EQ("Mon"s, to_string(days{4}));
    ASSERT_EQ("Tue"s, to_string(days{5}));
    ASSERT_EQ("Wed"s, to_string(days{6}));
    ASSERT_EQ("Thu"s, to_string(days{7}));
    ASSERT_EQ("Fri"s, to_string(days{8}));
    ASSERT_EQ("Sat"s, to_string(days{9}));
    ASSERT_EQ("Sun"s, to_string(days{10}));
    ASSERT_EQ("Mon"s, to_string(days{11}));
    ASSERT_EQ("Tue"s, to_string(days{12}));
    ASSERT_EQ("Wed"s, to_string(days{13}));
}

TEST(StdExtension,TimePoint2RFC1123)
{
    ASSERT_EQ("Thu, 01 Jan 1970 00:00:00 GMT"s, to_rfc1123(system_clock::time_point{0us}));
    ASSERT_EQ("Thu, 01 Jan 1970 00:00:00 GMT"s, to_rfc1123(system_clock::time_point{1ms}));
    ASSERT_EQ("Thu, 01 Jan 1970 00:00:01 GMT"s, to_rfc1123(system_clock::time_point{1s}));
    ASSERT_EQ("Thu, 01 Jan 1970 00:01:00 GMT"s, to_rfc1123(system_clock::time_point{1min}));
    ASSERT_EQ("Thu, 01 Jan 1970 01:00:00 GMT"s, to_rfc1123(system_clock::time_point{1h}));
    ASSERT_EQ("Thu, 01 Jan 1970 12:00:00 GMT"s, to_rfc1123(system_clock::time_point{12h}));
}

TEST(StdExtension,TimePoint2ISO8601)
{
    ASSERT_EQ("1970-01-01T00:00:00.000Z"s, to_iso8601(system_clock::time_point{0us}));
    ASSERT_EQ("1970-01-01T00:00:00.001Z"s, to_iso8601(system_clock::time_point{1ms}));
    ASSERT_EQ("1970-01-01T00:00:01.000Z"s, to_iso8601(system_clock::time_point{1s}));
    ASSERT_EQ("1970-01-01T00:01:00.000Z"s, to_iso8601(system_clock::time_point{1min}));
    ASSERT_EQ("1970-01-01T01:00:00.000Z"s, to_iso8601(system_clock::time_point{1h}));
    ASSERT_EQ("1970-01-01T12:00:00.000Z"s, to_iso8601(system_clock::time_point{12h}));
}

TEST(StdExtension,TimePoint2String)
{
    ASSERT_EQ("1970-01-01T00:00:00.000Z"s, to_string(system_clock::time_point{0us}));
    ASSERT_EQ("1970-01-01T00:00:00.001Z"s, to_string(system_clock::time_point{1ms}));
    ASSERT_EQ("1970-01-01T00:00:01.000Z"s, to_string(system_clock::time_point{1s}));
    ASSERT_EQ("1970-01-01T00:01:00.000Z"s, to_string(system_clock::time_point{1min}));
    ASSERT_EQ("1970-01-01T01:00:00.000Z"s, to_string(system_clock::time_point{1h}));
    ASSERT_EQ("1970-01-01T12:00:00.000Z"s, to_string(system_clock::time_point{12h}));
}

TEST(StdExtension,String2TimePoint)
{
    ASSERT_EQ(system_clock::time_point{0us}, stotp("1970-01-01T00:00:00.000Z"s));
    ASSERT_EQ(system_clock::time_point{1ms}, stotp("1970-01-01T00:00:00.001Z"s));
    ASSERT_EQ(system_clock::time_point{1s}, stotp("1970-01-01T00:00:01.000Z"s));
    ASSERT_EQ(system_clock::time_point{1min}, stotp("1970-01-01T00:01:00.000Z"s));
    ASSERT_EQ(system_clock::time_point{1h}, stotp("1970-01-01T01:00:00.000Z"s));
    ASSERT_EQ(system_clock::time_point{12h}, stotp("1970-01-01T12:00:00.000Z"s));
}

TEST(StdExtension,Boolean2String)
{
    ASSERT_EQ("true"s, to_string(true));
    ASSERT_EQ("false"s, to_string(false));
}

TEST(StdExtension,String2Boolean)
{
    ASSERT_EQ(true, stob("true"s));
    ASSERT_EQ(true, stob("1"s));
    ASSERT_EQ(false, stob("false"s));
    ASSERT_EQ(false, stob("0"s));
}

TEST(StdExtension,Nullptr2String)
{
    ASSERT_EQ("null"s, to_string(nullptr));
}

TEST(StdExtension,String2String)
{
    ASSERT_EQ("abcdefg1234567"s, to_string("abcdefg1234567"s));
}
