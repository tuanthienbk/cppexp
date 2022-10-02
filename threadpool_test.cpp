#include "threadpool.h"

#include <gtest/gtest.h>

TEST(threadpool, test1)
{
    using namespace std::literals;
    std::vector<int> v = {1, 2, 3, 4, 5};
    parallel_for(v.begin(), v.end(), [](auto&& val)
    { 
        // std::this_thread::sleep_for(10ms);
        std::cout << val << std::endl;
        // return val * val;
    });
    std::this_thread::sleep_for(1000ms);
    ASSERT_EQ(1, 1);
}
