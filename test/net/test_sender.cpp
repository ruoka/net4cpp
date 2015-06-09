#include <iostream>
#include <system_error>
#include <gtest/gtest.h>
#include "net/sender.hpp"

using namespace std;
using namespace net;

TEST(SenderTest,commandLine)
try
{
    sender sder{"228.0.0.4","54321"};
    ostream os{sder.distribute()};
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
    clog << "Exception: " << e.what() << endl;
}
