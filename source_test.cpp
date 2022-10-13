#include "source.h"

#include <gtest/gtest.h>

TEST(source, test1)
{
    source<int> s(0);
    // s.subscribe([](const int& i) { std::cout << i << std::endl;});
    s.bind([](const auto& i){ std::cout << std::get<0>(i) << std::endl;});
    s = 1;
    s = 2;
}

TEST(source, test2)
{
    source<int, float> s(1, 0.1);
    s.subscribe([](const int& i, const float& j) { std::cout << i << " " << j << std::endl;});
    s.set<0>(2);
    s.set<1>(0.2);
}