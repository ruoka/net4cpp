#include <thread>
#include <iostream>
#include <vector>
#include <gtest/gtest.h>
#include "net/sender.hpp"
#include "net/receiver.hpp"

using namespace std;
using namespace net;

static mutex m;

TEST(ReceiverAndSenderTest,runTwoThreads)
{
    int s{0};
    int test[1000];
    for(auto& i : test)
        i = ++s;

    thread t1{
        [&test]{
            receiver rver{"228.0.0.4", "54321"};
            istream is{rver.join()};
            {
                unique_lock<mutex> l{m};
                clog << "Listening: " << rver.group() << '.' << rver.service() << endl;
            }
            for(auto i : test)
            {
                int ii;
                is >> ii;
                ASSERT_EQ(i,ii);
                unique_lock<mutex> l{m};
                clog << "lter: " << ii << endl;
            }
        }
    };

    thread t2{
        [&test]{
            sender sder{"228.0.0.4", "54321"};
            ostream os{sder.distribute()};
            for(auto i : test)
            {
                this_thread::sleep_for(1ms);
                os << i << endl;
                unique_lock<mutex> l{m};
                clog << "mter: " << i << endl;
            }
        }
    };

    t1.join();
    t2.join();
}

TEST(ReceiverAndSenderTest,runManyThreads)
{
    int s{0};
    int test[1000];
    for(auto& i : test)
        i = ++s;

    vector<thread> threads;

    for(int i = 100; --i; i)
        threads.push_back(thread{
            [&test]{
                receiver rver{"228.0.0.4", "54321"};
                istream is{rver.join()};
                {
                    unique_lock<mutex> l{m};
                    clog << "Listening: " << rver.group() << '.' << rver.service() << endl;
                }
                for(auto i : test)
                {
                    int ii;
                    is >> ii;
                    ASSERT_EQ(i,ii);
                    unique_lock<mutex> l{m};
                    clog << "lter: " << ii << endl;
                }
            }
        }
        );

    threads.push_back(thread{
        [&test]{
            sender sder{"228.0.0.4", "54321"};
            ostream os{sder.distribute()};
            for(auto i : test)
            {
                this_thread::sleep_for(1ms);
                os << i << endl;
                unique_lock<mutex> l{m};                
                clog << "mter: " << i << endl;
            }
        }
    }
    );

    for(auto& thr : threads)
        thr.join();
}
