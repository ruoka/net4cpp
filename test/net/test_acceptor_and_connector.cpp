#include <array>
#include <future>
#include <gtest/gtest.h>
#include "net/acceptor.hpp"
#include "net/connector.hpp"

using namespace std;
using namespace net;

class NetAcceptorAndConnectorTest : public ::testing::Test
{
protected:

    void SetUp()
    {
        auto s = 0;
        for(auto& i : test)
            i = ++s;
    }

    array<int,100> test;

    mutex m;
};

TEST_F(NetAcceptorAndConnectorTest,RunTwoThreads)
{
    auto f1 = async(
        launch::async,
        [&]{
            auto ator = acceptor{"localhost", "54321"};
            {
                auto l = unique_lock<mutex>{m};
                SUCCEED() << "Accepting...  " << ator.host() << '.' << ator.service_or_port() << endl;
            }
            auto os = ator.accept();
            for(auto i : test)
            {
                os << i << endl;
                auto l = unique_lock<mutex>{m};
                SUCCEED() << "Acceptor:  " << i << endl;
            }
        });

    auto f2 = async(
        launch::async,
        [&]{
            auto ctor = connector{"localhost", "54321"};
            {
                auto l = unique_lock<mutex>{m};
                SUCCEED() << "Connecting... " << ctor.host() << '.' << ctor.service_or_port() << endl;
            }
            auto is = ctor.connect();
            for(auto i : test)
            {
                auto ii = 0;
                is >> ii;
                ASSERT_EQ(i,ii);
                auto l = unique_lock<mutex>{m};
                SUCCEED() << "Connector: " << ii << endl;
            }
        });

    f1.get();
    f2.get();
}
