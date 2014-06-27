#include <iostream>
#include <gtest/gtest.h>
#include "net/receiver.hpp"

using namespace std;
using namespace net;

TEST(ReceiverTest,commandLine)
{
    try
    {
        receiver rver{"228.0.0.4","54321"};
        istream is{rver.join()};
        clog << "Listening: " << rver.group() << '.' << rver.service() << endl;
        while(cout && is)
        {
            string msg;
            getline(is,msg);
            clog << msg << endl;
        }
        if(errno)
            throw std::system_error{errno, std::system_category(), "Errno"};
    }
    catch(exception& e)
    {
        clog << e.what() << endl;
    }
}
