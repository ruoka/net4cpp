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
    ASSERT_EQ("Jan"s, to_string(month{1}));
    ASSERT_EQ("Feb"s, to_string(month{2}));
    ASSERT_EQ("Mar"s, to_string(month{3}));
    ASSERT_EQ("Apr"s, to_string(month{4}));
    ASSERT_EQ("May"s, to_string(month{5}));
    ASSERT_EQ("Jun"s, to_string(month{6}));
    ASSERT_EQ("Jul"s, to_string(month{7}));
    ASSERT_EQ("Aug"s, to_string(month{8}));
    ASSERT_EQ("Sep"s, to_string(month{9}));
    ASSERT_EQ("Oct"s, to_string(month{10}));
    ASSERT_EQ("Nov"s, to_string(month{11}));
    ASSERT_EQ("Dec"s, to_string(month{12}));
}

TEST(StdExtension,Weekday2String)
{
    ASSERT_EQ("Sun"s, to_string(weekday{0}));
    ASSERT_EQ("Mon"s, to_string(weekday{1}));
    ASSERT_EQ("Tue"s, to_string(weekday{2}));
    ASSERT_EQ("Wed"s, to_string(weekday{3}));
    ASSERT_EQ("Thu"s, to_string(weekday{4}));
    ASSERT_EQ("Fri"s, to_string(weekday{5}));
    ASSERT_EQ("Sat"s, to_string(weekday{6}));
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
    ASSERT_EQ(system_clock::time_point{0us}, to_time_point("1970-01-01T00:00:00.000Z"s));
    ASSERT_EQ(system_clock::time_point{1ms}, to_time_point("1970-01-01T00:00:00.001Z"s));
    ASSERT_EQ(system_clock::time_point{1s}, to_time_point("1970-01-01T00:00:01.000Z"s));
    ASSERT_EQ(system_clock::time_point{1min}, to_time_point("1970-01-01T00:01:00.000Z"s));
    ASSERT_EQ(system_clock::time_point{1h}, to_time_point("1970-01-01T01:00:00.000Z"s));
    ASSERT_EQ(system_clock::time_point{12h}, to_time_point("1970-01-01T12:00:00.000Z"s));
    ASSERT_EQ(system_clock::time_point{days{1}}, to_time_point("1970-01-02T00:00:00.000Z"s));
//  ASSERT_EQ(system_clock::time_point{months{1}}, to_time_point("1970-02-01T00:00:00.000Z"s));
//  ASSERT_EQ(system_clock::time_point{years{1}}, to_time_point("1971-01-01T00:00:00.000Z"s));
}

TEST(StdExtension,Boolean2String)
{
    ASSERT_EQ("true"s, to_string(true));
    ASSERT_EQ("false"s, to_string(false));
}

TEST(StdExtension,String2Boolean)
{
    ASSERT_EQ(true, to_boolean("true"s));
    ASSERT_EQ(true, to_boolean("1"s));
    ASSERT_EQ(false, to_boolean("false"s));
    ASSERT_EQ(false, to_boolean("0"s));
}

TEST(StdExtension,Nullptr2String)
{
    ASSERT_EQ("null"s, to_string(nullptr));
}

TEST(StdExtension,String2String)
{
    ASSERT_EQ("abcdefg1234567"s, to_string("abcdefg1234567"s));
}

TEST(StdExtension,StringView2Integer)
{
    auto test = "12345"sv;
    ASSERT_EQ(12345, stoi(test));
    ASSERT_EQ(123, stoi(test.substr(0,3)));
    ASSERT_EQ(12345u, stou(test));
    ASSERT_EQ(123u, stou(test.substr(0,3)));
    ASSERT_EQ(12345l, stol(test));
    ASSERT_EQ(123l, stol(test.substr(0,3)));
    ASSERT_EQ(12345ll, stoll(test));
    ASSERT_EQ(123ll, stoll(test.substr(0,3)));
}
