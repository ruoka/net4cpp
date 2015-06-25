#include <iostream>
#include <gtest/gtest.h>
#include "net/connector.hpp"
#include "net/address_info.hpp"
#include "net/socket.hpp"

using namespace std;
using namespace net;

TEST(ConnectorTest,Construct)
{
    connector ctor{"google.com","http"};
    ASSERT_EQ(ctor.host(),"google.com");
    ASSERT_EQ(ctor.service(),"http");
    ASSERT_EQ(ctor.timeout(),default_connect_timeout);
}

TEST(ConnectorTest,Connect)
{
    EXPECT_NO_THROW({
        auto s = connect("google.com","http");
        ASSERT_FALSE(!s);
    });
}

TEST(ConnectorTest,Timeout)
{
    const net::address_info address{"localhost", "1999", SOCK_STREAM, AI_PASSIVE};
    const net::socket s{address->ai_family, address->ai_socktype, address->ai_protocol};
    net::bind(s, address->ai_addr, address->ai_addrlen);
    connector ctor{"localhost","1999"};
    ctor.timeout(3s);
    ASSERT_THROW(endpointstream eps{ctor.connect()}, system_error);
}

TEST(ConnectorTest,Fail2Connect)
{
    connector ctor{"foo.bar","http"};
    ASSERT_THROW(endpointstream eps{ctor.connect()}, system_error);
}

TEST(ConnectorTest,CommandLine)
try
{
    connector ctor{"localhost","1999"};
    auto s = ctor.connect();
    while(cin && s)
    {
        string echo;
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

