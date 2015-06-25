#include <gtest/gtest.h>
#include <iostream>
#include "net/acceptor.hpp"

using namespace std;
using namespace net;

TEST(AcceptorTest,Construct)
{
    acceptor ator{"localhost","54321"};
    ASSERT_EQ(ator.host(),"localhost");
    ASSERT_EQ(ator.service(),"54321");
    ASSERT_EQ(ator.timeout(),default_accept_timeout);
}

TEST(AcceptorTest,Fail2Construct)
{
    ASSERT_THROW((acceptor{"google.com","http"}), system_error);
}

TEST(AcceptorTest,Timeout)
{
    acceptor ator{"1999"};
    ator.timeout(1s);
    EXPECT_THROW(endpointstream eps{ator.accept()}, system_error);
}

TEST(AcceptorTest,CommandLine)
{
    acceptor ator{"1999"};
    while(true)
    {
        auto s = ator.accept();
        s << "Welcome to Hello Yellow Echo Server!" << endl;
        while(s)
        {
            string echo;
            getline(s, echo);
            s << echo << endl;
            clog << echo << endl;
        }
    }
}
