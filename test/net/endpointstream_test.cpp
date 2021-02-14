#include <sstream>
#include <gtest/gtest.h>
#include "net/endpointstream.hpp"
#include "net/sender.hpp"
#include "net/receiver.hpp"
#include "net/connector.hpp"

using namespace net;
using namespace std;
using namespace std::literals;

TEST(NetEndpointStreamTest,Construct1)
{
    auto is = join("228.0.0.4","54321");
}

TEST(NetEndpointStreamTest,Construct2)
{
    auto os = distribute("228.0.0.4","54321");
}

TEST(NetEndpointStreamTest,HttpRequestAndResponse)
{
    auto s = connect("www.google.fi","http");

    s << "GET / HTTP/1.1"                << crlf
      << "Host: www.google.com"          << crlf
      << "Connection: close"             << crlf
      << "Accept: text/plain, text/html" << crlf
      << "Accept-Charset: utf-8"         << crlf
      << crlf
      << flush;

    EXPECT_TRUE(s.wait_for(1s));

    auto os = ostringstream{};
    while(s)
    {
        char c;
        s >> noskipws >> c;
        os << c;
    }
    SUCCEED() << os.str();
    cout << os.str();;
}

TEST(NetEndpointStreamTest,WaitFor)
{
    auto s = connect("www.google.fi","http");
    EXPECT_FALSE(s.wait_for(1s));
}

TEST(NetEndpointStreamTest,Move)
{
    auto s1 = net::endpointstream{nullptr};
    s1 = connect("www.google.fi","http");;
}
