#include <iostream>
#include <gtest/gtest.h>
#include "net/receiver.hpp"

using namespace std;
using namespace net;

TEST(NetReceiverTest,Construct)
{
    auto rver = receiver{"228.0.0.4","test"};
    ASSERT_EQ(rver.group(),"228.0.0.4");
    ASSERT_EQ(rver.service(),"test");
}

TEST(NetReceiverTest,Join)
{
    auto s = join("228.0.0.4", "54321");
    ASSERT_FALSE(!s);
}

TEST(NetReceiverTest,CommandLine)
try
{
    auto rver = receiver{"228.0.0.4","54321"};
    auto is = rver.join();
    clog << "Receiver: " << rver.group() << '.' << rver.service() << endl;
    while(cout && is)
    {
        auto msg = ""s;
        getline(is, msg);
        clog << msg << endl;
    }
    if(errno)
        throw system_error{errno, system_category(), "Errno"};
}
catch(const exception& e)
{
    cerr << e.what() << endl;
}
