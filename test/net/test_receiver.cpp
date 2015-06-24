#include <iostream>
#include <gtest/gtest.h>
#include "net/receiver.hpp"

using namespace std;
using namespace net;

TEST(ReceiverTest,commandLine)
try
{
    receiver rver{"228.0.0.4","54321"};
    auto is = rver.join();
    clog << "Receiver: " << rver.group() << '.' << rver.service() << endl;
    while(cout && is)
    {
        string msg;
        getline(is, msg);
        clog << msg << endl;
    }
    if(errno)
        throw system_error{errno, system_category(), "Errno"};
}
catch(const exception& e)
{
    clog << e.what() << endl;
}
