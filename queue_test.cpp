#include "concurrent_queue.h"

#include <gtest/gtest.h>
#include <thread>

TEST(concurrent_queue, test1)
{
    concurrent_queue<int> q;
    for(int i = 0; i < 100; i++) q.push(i);
    std::vector<int> v1, v2;
    std::thread t1([&]() 
    { 
        while(1)
        {
            auto val = q.pop();
            if (val) 
                v1.push_back(val.value());
            else
                break;
        }
    });
    std::thread t2([&]() 
    { 
        while(1)
        {
            auto val = q.pop();
            if (val) 
                v2.push_back(val.value());
            else break;
        }
    });
    t1.join();
    t2.join();
    std::set<int> s;
    for(auto& val : v1)
    {
        if (s.find(val) != s.end()) 
            ASSERT_TRUE(0);
        else
            s.insert(val);
    }
    for(auto& val : v2)
    {
        if (s.find(val) != s.end()) 
            ASSERT_TRUE(0);
        else 
            s.insert(val);
    }
    ASSERT_EQ(1,1);
}