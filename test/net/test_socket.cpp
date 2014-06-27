#include <iostream>
#include <gtest/gtest.h>
#include "net/socket.hpp"
#include "net/network.hpp"

using namespace std;
using namespace net;

TEST(SocketUnitTest,testConstructor)
{
    const net::socket s{AF_INET, SOCK_STREAM};
    int fd = s;
    clog << fd << endl;
    ASSERT_FALSE(!s);
    ASSERT_GT(fd,0);
    int yes{1};
    auto e = net::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    clog << e << endl;
    ASSERT_EQ(e,0);
}

TEST(SocketUnitTest,testMove)
{
    net::socket s1{AF_INET, SOCK_DGRAM};
    int fd1 = s1;
    clog << fd1 << endl;
    ASSERT_FALSE(!s1);
    ASSERT_GT(fd1,0);
    const net::socket s2{std::move(s1)};
    int fd2 = s2;
    clog << fd2 << endl;    
    ASSERT_FALSE(!s2);
    ASSERT_TRUE(!s1);
    ASSERT_EQ(fd2,fd1);
    fd1 = s1;
    clog << fd1 << endl;
    ASSERT_EQ(fd1,-1);
}

TEST(SocketUnitTest,testWaitFor)
{
    const net::socket s{AF_INET, SOCK_STREAM};
    auto b = s.wait_for(1s);
    ASSERT_FALSE(b);
}
