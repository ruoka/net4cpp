#include <gtest/gtest.h>
#include <iostream>
#include "net/acceptor.hpp"

using namespace std;
using namespace net;

TEST(AcceptorUnitTest,testConstruct)
{
    acceptor ator{"localhost","54321"};
    ASSERT_EQ(ator.host(),"localhost");
    ASSERT_EQ(ator.service(),"54321");
    ASSERT_EQ(ator.timeout(),default_accept_timeout);
}

TEST(AcceptorUnitTest,testConstructFails)
{
    ASSERT_THROW((acceptor{"google.com","http"}), system_error);
}

TEST(AcceptorUnitTest,testTimeout)
{
    acceptor ator{"1999"};
    ator.timeout(chrono::seconds{1});
    ASSERT_THROW(iostream ios{ator.accept()}, system_error);
}

TEST(AcceptorTest,commandLine)
{
    acceptor ator{"1999"};
    while(true)
    {
        iostream ios{ator.accept()};
        ios << "Welcome to Hello Yellow Echo Server!" << endl;
        while(ios)
        {
            string echo;
            getline(ios,echo);
            ios << echo << endl;
            clog << echo << endl;
        }
    }
}
