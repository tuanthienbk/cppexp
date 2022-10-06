#include "task.h"

#include <gtest/gtest.h>

async<int> fn1()
{
    using namespace std::literals;
    auto result  = co_await task([]() -> int 
    { 
        std::this_thread::sleep_for(100ms); 
        return 1; 
    });
    co_return result;
}
async<int> fn2()
{
    using namespace std::literals;
    auto result  = co_await task([]() -> int 
    { 
        std::this_thread::sleep_for(200ms); 
        return 2; 
    });
    co_return result;
}

async<int> test()
{
    auto v1 = fn1();
    auto v2 = fn2();
    co_return (co_await v1) + (co_await v2);
}

TEST(task, test1)
{
    test();
}