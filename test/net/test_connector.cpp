#include <iostream>
#include <gtest/gtest.h>
#include "net/connector.hpp"
#include "net/address_info.hpp"
#include "net/socket.hpp"

using namespace std;
using namespace net;

TEST(ConnectorUnitTest,testConstruct)
{
    connector ctor{"google.com","http"};
    ASSERT_EQ(ctor.host(),"google.com");
    ASSERT_EQ(ctor.service(),"http");
    ASSERT_EQ(ctor.timeout(),default_connect_timeout);
}

TEST(ConnectorUnitTest,testTimeout)
{
    const net::address_info address{"localhost", "1999", SOCK_STREAM, AI_PASSIVE};
    const net::socket s{address->ai_family, address->ai_socktype, address->ai_protocol};
    net::bind(s, address->ai_addr, address->ai_addrlen);

    connector ctor{"localhost","1999"};
    ctor.timeout(chrono::seconds{3});

    ASSERT_THROW(iostream ios{ctor.connect()}, system_error);
}

TEST(ConnectorUnitTest,testFailingToConnect)
{
    connector ctor{"foo.bar","http"};
    ASSERT_THROW(iostream ios{ctor.connect()}, system_error);
}

TEST(ConnectorUnitTest,testHttpRequest)
{
    connector ctor{"www.google.com","http"};

    iostream ios{ctor.connect()};

    ios << "GET / HTTP/1.1\r\n"
        << "Host: www.google.com\r\n"
        << "Connection: close\r\n"
        << "Accept: text/plain, text/html\r\n"
        << "Accept-Charset: utf-8\r\n"
        << "\r\n"
        << flush;

    while(ios)
    {
        char c;
        ios >> noskipws >> c;
        clog << c;
    }
}

TEST(ConnectorTest,commandLine)
{
    try
    {
        connector ctor{"localhost","1999"};
        iostream ios{ctor.connect()};
        while(cin && ios)
        {
            string echo;
            getline(cin,echo);
            ios << echo << endl;
            getline(ios,echo);
            clog << echo << endl;
        }
    }
    catch(const exception& e)
    {
        clog << "Exception: " << e.what() << endl;
    }
}
