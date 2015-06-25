#include <sstream>
#include <gtest/gtest.h>
#include "net/endpointstream.hpp"
#include "net/sender.hpp"
#include "net/receiver.hpp"
#include "net/connector.hpp"

using namespace net;
using namespace std;

TEST(IEndpointStreamTest,Construct)
{
    auto is{join("228.0.0.4","54321")};
}

TEST(OEndpointStreamTest,Construct)
{
    auto os{distribute("228.0.0.4","54321")};
}

TEST(EndpointStreamTest,HttpRequestAndResponse)
{
    auto s = connect("www.google.fi","http");

    s << "GET / HTTP/1.1\r\n"
      << "Host: www.google.com\r\n"
      << "Connection: close\r\n"
      << "Accept: text/plain, text/html\r\n"
      << "Accept-Charset: utf-8\r\n"
      << "\r\n"
      << flush;

    ostringstream os;
    while(s)
    {
        char c;
        s >> noskipws >> c;
        os << c;
    }
    SUCCEED() << os.str();
}
