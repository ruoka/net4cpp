#include <iostream>
#include <system_error>
#include <gtest/gtest.h>
#include "net/sender.hpp"

using namespace std;
using namespace net;

TEST(SenderTest,Construct)
{
    sender sder{"228.0.0.4","test"};
    ASSERT_EQ(sder.group(),"228.0.0.4");
    ASSERT_EQ(sder.service(),"test");
}

TEST(SenderTest,Distribute)
{
    auto s = distribute("228.0.0.4", "54321", 3);
    ASSERT_FALSE(!s);
}

TEST(SenderTest,CommandLine)
try
{
    sender sder{"228.0.0.4","54321"};
    auto os = sder.distribute();
    clog << "Sender: " << sder.group() << '.' << sder.service() << endl;
    while(cin && os)
    {
        string msg;
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
