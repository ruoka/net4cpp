#include <array>
#include <future>
#include <iostream>
#include <gtest/gtest.h>
#include "net/acceptor.hpp"
#include "net/connector.hpp"

using namespace std;
using namespace net;

static mutex m;

TEST(AcceptorAndConnectorTest,runTwoThreads)
{
    int s{0};
    std::array<int,1000> test;
    for(auto& i : test)
        i = ++s;

    auto f1 = async(
        launch::async,
        [&test]()
        {
            acceptor ator{"localhost", "54321"};
            ostream os{ator.accept()};
            for(auto i : test)
            {
                os << i << endl;
                unique_lock<mutex> l{m};
                clog << "ator: " << i << endl;
            }
        }
    );

    auto f2 = async(
        launch::async,
        [&test]()
        {
            connector ctor{"localhost", "54321"};
            istream is{ctor.connect()};
            clog << "Listening..." << endl;
            for(auto i : test)
            {
                int ii;
                is >> ii;
                ASSERT_EQ(i,ii);
                unique_lock<mutex> l{m};
                clog << "ctor: " << ii << endl;
            }
        }
    );

    f1.get();
    f2.get();
}
