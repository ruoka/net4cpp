#include <iostream>
#include <gtest/gtest.h>
#include "net/socket.hpp"
#include "net/network.hpp"

using namespace std;
using namespace net;

TEST(SocketTest,Construct1)
{
    const net::socket s{AF_INET, SOCK_STREAM};
    ASSERT_FALSE(!s);
    int fd = s;
    ASSERT_GT(fd,0);
    auto yes = 1;
    auto e = net::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    ASSERT_EQ(e,0);
}

TEST(SocketTest,Construct2)
{
    const net::socket s{2112};
    ASSERT_FALSE(!s);
    int fd = s;
    ASSERT_EQ(fd,2112);
    auto yes = 1;
    auto e = net::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    ASSERT_EQ(e,-1);
}

TEST(SocketTest,Move)
{
    net::socket s1{AF_INET, SOCK_DGRAM};
    ASSERT_FALSE(!s1);
    int fd1 = s1;
    ASSERT_GT(fd1,0);
    const net::socket s2{std::move(s1)};
    ASSERT_TRUE(!s1);
    ASSERT_FALSE(!s2);
    int fd2 = s2;
    ASSERT_EQ(fd2,fd1);
    fd1 = s1;
    ASSERT_EQ(fd1,-1);
}

TEST(SocketTest,WaitFor)
{
    const net::socket s{AF_INET, SOCK_STREAM};
    auto b = s.wait_for(1s);
    ASSERT_FALSE(b);
}
