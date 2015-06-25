#include <gtest/gtest.h>
#include "net/address_info.hpp"

using namespace std;
using namespace net;

TEST(AddressInfoTest,Construct)
{
    net::address_info ai{"localhost", "54321", SOCK_STREAM};
    auto b = begin(ai);
    auto e = end(ai);
    ASSERT_TRUE(b != e);
    for(const auto& a : ai)
        ASSERT_EQ(a.ai_socktype, SOCK_STREAM);
}
