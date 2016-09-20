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
