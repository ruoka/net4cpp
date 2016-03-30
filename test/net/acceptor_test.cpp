#include <thread>
#include <iostream>
#include <gtest/gtest.h>
#include "net/acceptor.hpp"
#include "net/connector.hpp"

using namespace std;
using namespace net;

TEST(NetAcceptorTest,Construct)
{
    acceptor ator{"localhost","54321"};
    EXPECT_EQ(ator.host(),"localhost");
    EXPECT_EQ(ator.service_or_port(),"54321");
    EXPECT_EQ(ator.timeout(),default_accept_timeout);
}

TEST(NetAcceptorTest,Fail2Construct)
{
    EXPECT_THROW((acceptor{"google.com","http"}), system_error);
}

TEST(NetAcceptorTest,Accept)
{
    thread t1{[]{
        acceptor ator{"localhost","50001"};
        string host, port;
        auto c = ator.accept(host, port);
        EXPECT_EQ(host,"localhost");
        EXPECT_GT(port,"49152");
        EXPECT_LT(port,"65535");
        SUCCEED() << "Connection: " << host << "." << port << endl;
    }};

    this_thread::sleep_for(10ms);

    thread t2{[]{
        auto h = connect("localhost","50001");
        EXPECT_FALSE(!h);
    }};

    t1.join();
    t2.join();
}

TEST(NetAcceptorTest,Timeout)
{
    acceptor ator{"1999"};
    ator.timeout(1s);
    EXPECT_THROW(endpointstream eps{ator.accept()}, system_error);
}

TEST(NetAcceptorTest,CommandLine)
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
