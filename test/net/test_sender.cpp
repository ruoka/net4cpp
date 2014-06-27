#include <thread>
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
    clog << "Distributing: " << sder.group() << '.' << sder.service() << endl;
    while(cin && os)
    {
        string echo;
        getline(cin,echo);
        os << echo << endl;
    }
    if(errno)
        throw std::system_error{errno, std::system_category(), "Errno"};
}
catch(exception& e)
{
    clog << "Exception: " << e.what() << endl;
}
