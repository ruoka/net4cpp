#include <thread>
#include <iostream>
#include <gtest/gtest.h>
#include "net/acceptor.hpp"
#include "net/connector.hpp"

using namespace std;
using namespace net;

TEST(NetAcceptorTest,Construct)
{
    auto ator = acceptor{"localhost","54321"};
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
    auto t1 = thread {
    []{
        auto ator = acceptor{"localhost","50001"};
        auto host = ""s;
        auto port = ""s;
        auto c = net::endpointstream{nullptr};
        EXPECT_NO_THROW(c = ator.accept(host, port));
        EXPECT_FALSE(!c);
        EXPECT_EQ(host,"localhost");
        EXPECT_GT(port,"49152");
        EXPECT_LT(port,"65535");
    }};

    this_thread::sleep_for(10ms);

    auto t2 = thread {
    []{
        auto s = net::endpointstream{nullptr};
        EXPECT_NO_THROW(s = connect("localhost","50001"));
        EXPECT_FALSE(!s);
    }};

    t1.join();
    t2.join();
}

TEST(NetAcceptorTest,Timeout)
{
    auto ator = acceptor{"1999"};
    ator.timeout(1s);
    EXPECT_THROW(endpointstream eps{ator.accept()}, system_error);
}

TEST(NetAcceptorTest,CommandLine)
{
    auto ator = acceptor{"1999"};
    while(true)
    {
        auto s = ator.accept();
        s << "Welcome to Hello Yellow Echo Server!" << endl;
        while(s)
        {
            auto echo = ""s;
            getline(s, echo);
            s << echo << endl;
            clog << echo << endl;
        }
    }
}
