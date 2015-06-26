#include <array>
#include <vector>
#include <thread>
#include <gtest/gtest.h>
#include "net/sender.hpp"
#include "net/receiver.hpp"

using namespace std;
using namespace net;

class ReceiverAndSenderTest : public ::testing::Test
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

TEST_F(ReceiverAndSenderTest,RunTwoThreads)
{
    thread t1{
        [&]{
            receiver rver{"228.0.0.4", "54321"};
            auto is = rver.join();
            {
                unique_lock<mutex> l{m};
                clog << "Listening: " << rver.group() << '.' << rver.service() << endl;
            }
            for(auto i : test)
            {
                int ii;
                is >> ii;
                EXPECT_EQ(i,ii);
                unique_lock<mutex> l{m};
                //clog << "Receiver: " << ii << endl;
            }
        }};

    this_thread::sleep_for(10ms);

    thread t2{
        [&]{
            sender sder{"228.0.0.4", "54321"};
            auto os = sder.distribute();
            for(auto i : test)
            {
                this_thread::sleep_for(1ms);
                os << i << endl;
                unique_lock<mutex> l{m};
                //clog << "Sender:   " << i << endl;
            }
        }};

    t1.join();
    t2.join();
}

TEST_F(ReceiverAndSenderTest,RunManyThreads)
{
    vector<thread> threads;

    for(auto i = 100; i > 0; --i)
        threads.push_back(thread{
            [&]{
                receiver rver{"228.0.0.4", "54321"};
                auto is = rver.join();
                {
                    unique_lock<mutex> l{m};
                    clog << "Listening: " << rver.group() << '.' << rver.service() << endl;
                }
                for(auto i : test)
                {
                    int ii;
                    is >> ii;
                    EXPECT_EQ(i,ii);
                    unique_lock<mutex> l{m};
                    //clog << "Receiver: " << ii << endl;
                }
            }});

    this_thread::sleep_for(10ms);

    threads.push_back(thread{
        [&]{
            sender sder{"228.0.0.4", "54321"};
            auto os = sder.distribute();
            for(auto i : test)
            {
                this_thread::sleep_for(1ms);
                os << i << endl;
                unique_lock<mutex> l{m};
                //clog << "Sender:   " << i << endl;
            }
        }});

    for(auto& thr : threads)
        thr.join();
}
