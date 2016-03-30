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
        int s{0};
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
            acceptor ator{"localhost", "54321"};
            {
                unique_lock<mutex> l{m};
                SUCCEED() << "Accepting...  " << ator.host() << '.' << ator.service_or_port() << endl;
            }
            auto os = ator.accept();
            for(auto i : test)
            {
                os << i << endl;
                unique_lock<mutex> l{m};
                SUCCEED() << "Acceptor:  " << i << endl;
            }
        });

    auto f2 = async(
        launch::async,
        [&]{
            connector ctor{"localhost", "54321"};
            {
                unique_lock<mutex> l{m};
                SUCCEED() << "Connecting... " << ctor.host() << '.' << ctor.service_or_port() << endl;
            }
            auto is = ctor.connect();
            for(auto i : test)
            {
                int ii;
                is >> ii;
                ASSERT_EQ(i,ii);
                unique_lock<mutex> l{m};
                SUCCEED() << "Connector: " << ii << endl;
            }
        });

    f1.get();
    f2.get();
}
