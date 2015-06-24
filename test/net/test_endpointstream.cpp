#include <gtest/gtest.h>
#include "net/endpointstream.hpp"
#include "net/connector.hpp"

using namespace net;
using namespace std;

TEST(EndpointStreamUnitTest,testConstruct)
{
    endpointstream es{connect("www.google.fi","http")};

    es << "GET / HTTP/1.1\r\n"
        << "Host: www.google.com\r\n"
        << "Connection: close\r\n"
        << "Accept: text/plain, text/html\r\n"
        << "Accept-Charset: utf-8\r\n"
        << "\r\n"
        << flush;

    while(es)
    {
        char c;
        es >> noskipws >> c;
        clog << c;
    }
    clog << flush;
}

