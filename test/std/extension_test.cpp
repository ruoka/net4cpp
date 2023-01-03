#include <gtest/gtest.h>
#include "std/extension.hpp"

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace ext;

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
