#include "net/address_info.hpp"
#include <gtest/gtest.h>

using namespace std;
using namespace net;

TEST(SocketUnitTest,testAddressInfo)
{
    net::address_info ai{"localhost", "54321", SOCK_STREAM};
    auto b = begin(ai);
    auto e = end(ai);
    ASSERT_TRUE(b != e);
    for(const auto& i : ai)
        ;
}
