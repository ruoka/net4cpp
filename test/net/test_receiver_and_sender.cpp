#include <array>
#include <vector>
#include <thread>
#include <mutex>
#include <gtest/gtest.h>
#include "net/sender.hpp"
#include "net/receiver.hpp"

using namespace std;
using namespace net;

class NetReceiverAndSenderTest : public ::testing::Test
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

TEST_F(NetReceiverAndSenderTest,RunTwoThreads)
{
    thread t1{
        [&]{
            auto rver = receiver{"228.0.0.4", "54321"};
            auto is = rver.join();
            {
                auto l = unique_lock<mutex>{m};
                SUCCEED() << "Listening: " << rver.group() << '.' << rver.service() << endl;
            }
            for(auto i : test)
            {
                auto ii = 0;
                is >> ii;
                EXPECT_EQ(i,ii);
                auto l = unique_lock<mutex>{m};
                //clog << "NetReceiver: " << ii << endl;
            }
        }};

    this_thread::sleep_for(10ms);

    thread t2{
        [&]{
            auto sder = sender{"228.0.0.4", "54321"};
            auto os = sder.distribute();
            for(auto i : test)
            {
                this_thread::sleep_for(1ms);
                os << i << endl;
                auto l = unique_lock<mutex>{m};
                //clog << "Sender:   " << i << endl;
            }
        }};

    t1.join();
    t2.join();
}

TEST_F(NetReceiverAndSenderTest,RunManyThreads)
{
    auto threads = vector<thread>{};

    for(auto i = 100; i > 0; --i)
        threads.push_back(thread{
            [&]{
                auto rver = receiver{"228.0.0.4", "54321"};
                auto is = rver.join();
                {
                auto l = unique_lock<mutex>{m};
                    //clog << "Listening: " << rver.group() << '.' << rver.service() << endl;
                }
                for(auto i : test)
                {
                    auto ii = 0;
                    is >> ii;
                    EXPECT_EQ(i,ii);
                auto l = unique_lock<mutex>{m};
                    //clog << "receiver: " << ii << endl;
                }
            }});

    this_thread::sleep_for(10ms);

    threads.push_back(thread{
        [&]{
            auto sder = sender{"228.0.0.4", "54321"};
            auto os = sder.distribute();
            for(auto i : test)
            {
                this_thread::sleep_for(1ms);
                os << i << endl;
                auto l = unique_lock<mutex>{m};
                //clog << "Sender:   " << i << endl;
            }
        }});

    for(auto& thr : threads)
        thr.join();
}
