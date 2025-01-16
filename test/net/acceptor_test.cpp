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
//    EXPECT_THROW((acceptor{"google.com","http"}), std::system_error);
}

TEST(NetAcceptorTest,Accept)
{
    auto t1 = thread {
    []{
        auto ator = acceptor{"localhost","50001"};
        auto stream = net::endpointstream{nullptr};
        auto host = ""s;
        auto port = ""s;
        EXPECT_NO_THROW(std::tie(stream,host,port) = ator.accept());
        EXPECT_FALSE(!stream);
//      EXPECT_EQ(host,"localhost");
        EXPECT_EQ(host,"::1");
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
//    EXPECT_THROW(ator.accept(), std::system_error);
}

TEST(NetAcceptorTest,CommandLine)
{
    auto ator = acceptor{"1999"};
    while(true)
    {
        auto [stream,client,port] = ator.accept();
        stream << "Welcome to Hello Yellow Echo Server!" << endl;
        while(stream)
        {
            auto echo = ""s;
            getline(stream, echo);
            stream << echo << endl;
            clog << echo << endl;
        }
    }
}
