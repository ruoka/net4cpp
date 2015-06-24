#include <gtest/gtest.h>
#include "net/endpointstream.hpp"
#include "net/sender.hpp"
#include "net/receiver.hpp"
#include "net/connector.hpp"

using namespace net;
using namespace std;

TEST(IEndpointStreamUnitTest,testConstruct)
{
    auto is{join("228.0.0.4","54321")};
}

TEST(OEndpointStreamUnitTest,testConstruct)
{
    auto os{distribute("228.0.0.4","54321")};
}

TEST(EndpointStreamUnitTest,testConstruct)
{
    auto s = connect("www.google.fi","http");

    s << "GET / HTTP/1.1\r\n"
      << "Host: www.google.com\r\n"
      << "Connection: close\r\n"
      << "Accept: text/plain, text/html\r\n"
      << "Accept-Charset: utf-8\r\n"
      << "\r\n"
      << flush;

    while(s)
    {
        char c;
        s >> noskipws >> c;
        clog << c;
    }
    clog << flush;
}
