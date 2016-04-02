#include <iostream>
#include <system_error>
#include <gtest/gtest.h>
#include "net/sender.hpp"

using namespace std;
using namespace net;

TEST(NetSenderTest,Construct)
{
    auto sder = sender{"228.0.0.4","test"};
    ASSERT_EQ(sder.group(),"228.0.0.4");
    ASSERT_EQ(sder.service_or_port(),"test");
}

TEST(NetSenderTest,Distribute)
{
    auto s = distribute("228.0.0.4", "54321", 3);
    ASSERT_FALSE(!s);
}

TEST(NetSenderTest,CommandLine)
try
{
    auto sder = sender{"228.0.0.4","54321"};
    auto os = sder.distribute();
    clog << "Sender: " << sder.group() << '.' << sder.service_or_port() << endl;
    while(cin && os)
    {
        auto msg = ""s;
        getline(cin, msg);
        os << msg << endl;
    }
    if(errno)
        throw system_error{errno, system_category(), "Errno"};
}
catch(const exception& e)
{
    cerr << "Exception: " << e.what() << endl;
}

TEST(NetSenderTest,UDP)
{
    auto s = distribute("localhost", "syslog");
    ASSERT_FALSE(!s);
}
