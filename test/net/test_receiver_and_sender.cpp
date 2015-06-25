#include <thread>
#include <iostream>
#include <vector>
#include <gtest/gtest.h>
#include "net/sender.hpp"
#include "net/receiver.hpp"

using namespace std;
using namespace net;

static mutex m;

TEST(ReceiverAndSenderTest,RunTwoThreads)
{
    int s{0};
    int test[1000];
    for(auto& i : test)
        i = ++s;

    thread t1{
        [&test]{
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
        }
    };

    thread t2{
        [&test]{
            sender sder{"228.0.0.4", "54321"};
            auto os = sder.distribute();
            for(auto i : test)
            {
                this_thread::sleep_for(10ms);
                os << i << endl;
                unique_lock<mutex> l{m};
                //clog << "Sender: " << i << endl;
            }
        }
    };

    t1.join();
    t2.join();
}

TEST(ReceiverAndSenderTest,RunManyThreads)
{
    int s{0};
    int test[1000];
    for(auto& i : test)
        i = ++s;

    vector<thread> threads;

    for(int i = 100; i > 0; --i)
        threads.push_back(thread{
            [&test]{
                receiver rver{"228.0.0.4", "54321"};
                auto is = rver.join();
                {
                    unique_lock<mutex> l{m};
                    //clog << "Listening: " << rver.group() << '.' << rver.service() << endl;
                }
                for(auto i : test)
                {
                    int ii;
                    is >> ii;
                    EXPECT_EQ(i,ii);
                    unique_lock<mutex> l{m};
                    //clog << "Receiver: " << ii << endl;
                }
            }
        }
        );

    threads.push_back(thread{
        [&test]{
            sender sder{"228.0.0.4", "54321"};
            auto os = sder.distribute();
            for(auto i : test)
            {
                this_thread::sleep_for(10ms);
                os << i << endl;
                unique_lock<mutex> l{m};                
                //clog << "Sender: " << i << endl;
            }
        }
    }
    );

    for(auto& thr : threads)
        thr.join();
}
