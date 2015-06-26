#include <thread>
#include <iostream>
#include <gtest/gtest.h>
#include "net/acceptor.hpp"
#include "net/connector.hpp"

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

TEST(AcceptorTest,Accept)
{
    thread t1{[]{
        acceptor ator{"localhost","50001"};
        string host, port;
        auto c = ator.accept(host, port);
        ASSERT_EQ(host,"localhost");
        ASSERT_GT(port,"50000");
        ASSERT_LT(port,"65535");
        clog << "Connection: " << host << "." << port << endl;
    }};

    this_thread::sleep_for(10ms);

    thread t2{[]{
        auto h = connect("localhost","50001");
        ASSERT_FALSE(!h);
    }};

    t1.join();
    t2.join();
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
