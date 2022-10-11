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
    std::vector<int> v = {1, 2, 3, 4, 5};
    for (auto &i : range(10, 20) |
                       views::take(3) |
                    //    std::views::filter([](int i){ return 0 == i % 2;}) |
                       views::transform([](const int& i){ return i * i;})
        )
        std::cout << i << std::endl;
}