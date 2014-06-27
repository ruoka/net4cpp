#include <array>
#include <future>
#include <iostream>
#include <gtest/gtest.h>
#include "net/acceptor.hpp"
#include "net/connector.hpp"

using namespace std;
using namespace net;

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
                clog << "ator: " << i << endl;
                os << i << endl;
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
                clog << "ctor: " << ii << endl;
                ASSERT_EQ(i,ii);
            }
        }
    );

    f1.get();
    f2.get();
}
