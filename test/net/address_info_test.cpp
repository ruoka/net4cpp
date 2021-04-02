#include <gtest/gtest.h>
#include "net/address_info.hpp"

using namespace std;
using namespace net;

TEST(NetAddressInfoTest,ConstructStream)
{
    auto ai = address_info{"localhost", "54321", SOCK_STREAM};
    auto b = begin(ai);
    auto e = end(ai);
    ASSERT_TRUE(b != e);
    for(const auto& a : ai)
        ASSERT_EQ(a.ai_socktype, SOCK_STREAM);
}

TEST(NetAddressInfoTest,ConstructDgram)
{
    auto ai = address_info{"localhost", "54321", SOCK_DGRAM};
    auto b = begin(ai);
    auto e = end(ai);
    ASSERT_TRUE(b != e);
    for(const auto& a : ai)
        ASSERT_EQ(a.ai_socktype, SOCK_DGRAM);
}

TEST(NetAddressInfoTest,ConstructNoHost)
{
    auto ai = address_info{"", "54321", SOCK_STREAM};
    auto b = begin(ai);
    auto e = end(ai);
    ASSERT_TRUE(b != e);
    for(const auto& a : ai)
        ASSERT_EQ(a.ai_socktype, SOCK_STREAM);
}
