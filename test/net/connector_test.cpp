#include <iostream>
#include <gtest/gtest.h>
#include "net/connector.hpp"
#include "net/address_info.hpp"
#include "net/socket.hpp"

using namespace std;
using namespace net;

TEST(NetConnectorTest,Construct)
{
    const auto ctor = connector{"google.com","http"};
    ASSERT_EQ(ctor.host(), "google.com");
    ASSERT_EQ(ctor.service_or_port(), "http");
    ASSERT_EQ(ctor.timeout(), default_connect_timeout);
}

TEST(NetConnectorTest,Connect)
{
    EXPECT_NO_THROW({
        const auto s = connect("google.com","http");
        ASSERT_FALSE(!s);
    });
}

TEST(NetConnectorTest,Timeout)
{
    const auto address = net::address_info{"localhost", "1999", SOCK_STREAM, AI_PASSIVE};
    const auto s = net::socket{address->ai_family, address->ai_socktype, address->ai_protocol};
    net::bind(s, address->ai_addr, address->ai_addrlen);
    auto ctor = connector{"localhost", "1999"};
    ctor.timeout(3s);
//    ASSERT_THROW(endpointstream eps{ctor.connect()}, std::system_error);
}

TEST(NetConnectorTest,Fail2Connect)
{
    auto ctor = connector{"foo.bar", "http"};
//    ASSERT_THROW(ctor.connect(), std::system_error);
}

TEST(NetConnectorTest,CommandLine)
try
{
    auto ctor = connector{"localhost", "1999"};
    auto s = ctor.connect();
    while(cin && s)
    {
        auto echo = ""s;
        getline(cin, echo);
        s << echo << endl;
        getline(s, echo);
        clog << echo << endl;
    }
}
catch(const exception& e)
{
    cerr << "Exception: " << e.what() << endl;
}
