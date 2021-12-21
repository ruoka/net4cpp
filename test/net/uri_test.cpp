#include <iostream>
#include <gtest/gtest.h>
#include "net/uri.hpp"

using namespace std::string_literals;
using namespace net;

TEST(NetURI,Parse1)
{
    auto raw = "http://kake:passwd@www.appinf.com:88/sample/isin?eq=FI123456789#frags"s;

    auto parsed = net::uri{raw};

    std::cout << "URI:      " << raw             << std::endl;
    std::cout << "Absolute: " << parsed.absolute << std::endl;
    std::cout << "Scheme:   " << parsed.scheme   << std::endl;
    std::cout << "Userinfo: " << parsed.userinfo << std::endl;
    std::cout << "Host:     " << parsed.host     << std::endl;
    std::cout << "Port:     " << parsed.port     << std::endl;
    std::cout << "Path:     " << parsed.path     << std::endl;
    std::cout << "Path[0]:  " << parsed.path[0]  << std::endl;
    std::cout << "Path[1]:  " << parsed.path[1]  << std::endl;
    std::cout << "Path[2]:  " << parsed.path[2]  << std::endl;
    std::cout << "Query:    " << parsed.query    << std::endl;
    std::cout << "Fragment: " << parsed.fragment << std::endl;

    ASSERT_TRUE(parsed.absolute);
    ASSERT_EQ(parsed.scheme, "http");
    ASSERT_EQ(parsed.userinfo, "kake:passwd");
    ASSERT_EQ(parsed.host, "www.appinf.com");
    ASSERT_EQ(parsed.port, "88");
    ASSERT_EQ(parsed.path, "/sample/isin");
    ASSERT_EQ(parsed.path[0], "");
    ASSERT_EQ(parsed.path[1], "sample");
    ASSERT_EQ(parsed.path[2], "isin");
    ASSERT_EQ(parsed.query, "eq=FI123456789");
    ASSERT_EQ(parsed.fragment, "frags");
}

TEST(NetURI,Parse2)
{
    auto raw = "https://localhost/test/13"s;

    auto parsed = net::uri{raw};

    std::cout << "URI:      " << raw             << std::endl;
    std::cout << "Absolute: " << parsed.absolute << std::endl;
    std::cout << "Scheme:   " << parsed.scheme   << std::endl;
    std::cout << "Userinfo: " << parsed.userinfo << std::endl;
    std::cout << "Host:     " << parsed.host     << std::endl;
    std::cout << "Port:     " << parsed.port     << std::endl;
    std::cout << "Path:     " << parsed.path     << std::endl;
    std::cout << "Path[0]:  " << parsed.path[0]  << std::endl;
    std::cout << "Path[1]:  " << parsed.path[1]  << std::endl;
    std::cout << "Path[2]:  " << parsed.path[2]  << std::endl;
    std::cout << "Query:    " << parsed.query    << std::endl;
    std::cout << "Fragment: " << parsed.fragment << std::endl;

    ASSERT_TRUE(parsed.absolute);
    ASSERT_EQ(parsed.scheme, "https");
    ASSERT_EQ(parsed.userinfo, "");
    ASSERT_EQ(parsed.host, "localhost");
    ASSERT_EQ(parsed.port, "");
    ASSERT_EQ(parsed.path, "/test/13");
    ASSERT_EQ(parsed.path[0], "");
    ASSERT_EQ(parsed.path[1], "test");
    ASSERT_EQ(parsed.path[2], "13");
    ASSERT_EQ(parsed.query, "");
    ASSERT_EQ(parsed.fragment, "");
}

TEST(NetURI,Parse3)
{
    auto raw = "urn:ISSN:1535–3613"s;

    auto parsed = net::uri{raw};

    std::cout << "URI:      " << raw             << std::endl;
    std::cout << "Absolute: " << parsed.absolute << std::endl;
    std::cout << "Scheme:   " << parsed.scheme   << std::endl;
    std::cout << "Userinfo: " << parsed.userinfo << std::endl;
    std::cout << "Host:     " << parsed.host     << std::endl;
    std::cout << "Port:     " << parsed.port     << std::endl;
    std::cout << "Path:     " << parsed.path     << std::endl;
    std::cout << "Path[0]:  " << parsed.path[0]  << std::endl;
    // std::cout << "Path[1]:  " << parsed.path[1]  << std::endl;
    // std::cout << "Path[2]:  " << parsed.path[2]  << std::endl;
    std::cout << "Query:    " << parsed.query    << std::endl;
    std::cout << "Fragment: " << parsed.fragment << std::endl;
}

TEST(NetURI,Parse4)
{
    auto raw = "ftp://www.appinf.com:2112/sample/13#frags2"s;

    auto parsed = net::uri{raw};

    std::cout << "URI:      " << raw             << std::endl;
    std::cout << "Absolute: " << parsed.absolute << std::endl;
    std::cout << "Scheme:   " << parsed.scheme   << std::endl;
    std::cout << "Userinfo: " << parsed.userinfo << std::endl;
    std::cout << "Host:     " << parsed.host     << std::endl;
    std::cout << "Port:     " << parsed.port     << std::endl;
    std::cout << "Path:     " << parsed.path     << std::endl;
    std::cout << "Path[0]:  " << parsed.path[0]  << std::endl;
    std::cout << "Path[1]:  " << parsed.path[1]  << std::endl;
    std::cout << "Path[2]:  " << parsed.path[2]  << std::endl;
    std::cout << "Query:    " << parsed.query    << std::endl;
    std::cout << "Fragment: " << parsed.fragment << std::endl;
}

TEST(NetURI,Parse5)
{
    auto raw = "ftp://kake@www.appinf.com:2112"s;

    auto parsed = net::uri{raw};

    std::cout << "URI:      " << raw             << std::endl;
    std::cout << "Absolute: " << parsed.absolute << std::endl;
    std::cout << "Scheme:   " << parsed.scheme   << std::endl;
    std::cout << "Userinfo: " << parsed.userinfo << std::endl;
    std::cout << "Host:     " << parsed.host     << std::endl;
    std::cout << "Port:     " << parsed.port     << std::endl;
    std::cout << "Path:     " << parsed.path     << std::endl;
    // std::cout << "Path[0]:  " << parsed.path[0]  << std::endl;
    // std::cout << "Path[1]:  " << parsed.path[1]  << std::endl;
    // std::cout << "Path[2]:  " << parsed.path[2]  << std::endl;
    std::cout << "Query:    " << parsed.query    << std::endl;
    std::cout << "Fragment: " << parsed.fragment << std::endl;
}

TEST(NetURI,ParseRelative1)
{
    auto raw = "../../sample/55#frags3"s;

    auto parsed = net::uri{raw};

    std::cout << "URI:      " << raw             << std::endl;
    std::cout << "Absolute: " << parsed.absolute << std::endl;
    std::cout << "Scheme:   " << parsed.scheme   << std::endl;
    std::cout << "Userinfo: " << parsed.userinfo << std::endl;
    std::cout << "Host:     " << parsed.host     << std::endl;
    std::cout << "Port:     " << parsed.port     << std::endl;
    std::cout << "Path:     " << parsed.path     << std::endl;
    std::cout << "Path[0]:  " << parsed.path[0]  << std::endl;
    std::cout << "Path[1]:  " << parsed.path[1]  << std::endl;
    std::cout << "Path[2]:  " << parsed.path[2]  << std::endl;
    std::cout << "Query:    " << parsed.query    << std::endl;
    std::cout << "Fragment: " << parsed.fragment << std::endl;

    ASSERT_FALSE(parsed.absolute);
    ASSERT_EQ(parsed.scheme, "");
    ASSERT_EQ(parsed.userinfo, "");
    ASSERT_EQ(parsed.host, "");
    ASSERT_EQ(parsed.port, "");
    ASSERT_EQ(parsed.path, "../../sample/55");
    ASSERT_EQ(parsed.path[0], "..");
    ASSERT_EQ(parsed.path[1], "..");
    ASSERT_EQ(parsed.path[2], "sample");
    ASSERT_EQ(parsed.path[3], "55");
    ASSERT_EQ(parsed.query, "");
    ASSERT_EQ(parsed.fragment, "frags3");
}

TEST(NetURI,ParseRelative2)
{
    auto raw = "/sample/id?eg=13"s;

    auto parsed = net::uri{raw};

    std::cout << "URI:      " << raw             << std::endl;
    std::cout << "Absolute: " << parsed.absolute << std::endl;
    std::cout << "Scheme:   " << parsed.scheme   << std::endl;
    std::cout << "Userinfo: " << parsed.userinfo << std::endl;
    std::cout << "Host:     " << parsed.host     << std::endl;
    std::cout << "Port:     " << parsed.port     << std::endl;
    std::cout << "Path:     " << parsed.path     << std::endl;
    std::cout << "Path[0]:  " << parsed.path[0]  << std::endl;
    std::cout << "Path[1]:  " << parsed.path[1]  << std::endl;
    std::cout << "Path[2]:  " << parsed.path[2]  << std::endl;
    std::cout << "Query:    " << parsed.query    << std::endl;
    std::cout << "Fragment: " << parsed.fragment << std::endl;
}

TEST(NetURI,ParseInvalid)
{
    auto raw = "ftp//www.appinf.com/sample/13#frags2"s;

    auto parsed = net::uri{raw};

    std::cout << "URI:      " << raw             << std::endl;
    std::cout << "Absolute: " << parsed.absolute << std::endl;
    std::cout << "Scheme:   " << parsed.scheme   << std::endl;
    std::cout << "Userinfo: " << parsed.userinfo << std::endl;
    std::cout << "Host:     " << parsed.host     << std::endl;
    std::cout << "Port:     " << parsed.port     << std::endl;
    std::cout << "Path:     " << parsed.path     << std::endl;
    std::cout << "Path[0]:  " << parsed.path[0]  << std::endl;
    std::cout << "Path[1]:  " << parsed.path[1]  << std::endl;
    std::cout << "Path[2]:  " << parsed.path[2]  << std::endl;
    std::cout << "Query:    " << parsed.query    << std::endl;
    std::cout << "Fragment: " << parsed.fragment << std::endl;
}

TEST(NetURI,ParseRelative3)
{
    auto raw = "/xxx?tail=10"s;

    auto parsed = net::uri{raw};

    std::cout << "URI:      " << raw             << std::endl;
    std::cout << "Absolute: " << parsed.absolute << std::endl;
    std::cout << "Scheme:   " << parsed.scheme   << std::endl;
    std::cout << "Userinfo: " << parsed.userinfo << std::endl;
    std::cout << "Host:     " << parsed.host     << std::endl;
    std::cout << "Port:     " << parsed.port     << std::endl;
    std::cout << "Path:     " << parsed.path     << std::endl;
    std::cout << "Path[0]:  " << parsed.path[0]  << std::endl;
    std::cout << "Path[1]:  " << parsed.path[1]  << std::endl;
    std::cout << "Query:    " << parsed.query    << std::endl;
    std::cout << "Fragment: " << parsed.fragment << std::endl;
}

TEST(NetURI,PortWithoutHost)
{
    auto raw = "FIXT.1.1://KAKE@:2112/xxx?tail=10"s;

    auto parsed = net::uri{raw};

    std::cout << "URI:      " << raw             << std::endl;
    std::cout << "Absolute: " << parsed.absolute << std::endl;
    std::cout << "Scheme:   " << parsed.scheme   << std::endl;
    std::cout << "Userinfo: " << parsed.userinfo << std::endl;
    std::cout << "Host:     " << parsed.host     << std::endl;
    std::cout << "Port:     " << parsed.port     << std::endl;
    std::cout << "Path:     " << parsed.path     << std::endl;
    std::cout << "Path[0]:  " << parsed.path[0]  << std::endl;
    std::cout << "Path[1]:  " << parsed.path[1]  << std::endl;
    std::cout << "Query:    " << parsed.query    << std::endl;
    std::cout << "Fragment: " << parsed.fragment << std::endl;

    ASSERT_TRUE(parsed.absolute);
    ASSERT_EQ(parsed.scheme, "FIXT.1.1");
    ASSERT_EQ(parsed.userinfo, "KAKE");
    ASSERT_EQ(parsed.host, "");
    ASSERT_EQ(parsed.port, "2112");
    ASSERT_EQ(parsed.path, "/xxx");
    ASSERT_EQ(parsed.query, "tail=10");
    ASSERT_EQ(parsed.fragment, "");
}
