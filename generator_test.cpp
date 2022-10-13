#include "generator.h"

#include <ranges>
#include <gtest/gtest.h>

generator<int> range(int low, int high)
{
    for (int i = low; i < high; ++i)
        co_yield i;
}

TEST(generator, test1)
{
    for (auto &i : range(10, 20))
        std::cout << i << std::endl;
}

TEST(generator, test2)
{
    for (auto &i : range(10, 20) |
                       views::take(5) |
                       views::filter([](const int& i) -> bool { return 0 == i % 2;}) |
                       views::transform([](const int& i){ return i * i;})
        )
        std::cout << i << std::endl;
}

TEST(generator, test3)
{
    (range(10, 20) |
    views::take(5) |
    views::filter([](const int& i) -> bool { return 0 == i % 2;}) |
    views::transform([](const int& i){ return i * i;})).bind([](const int& i)
    {
        std::cout << i << std::endl;
    });
}