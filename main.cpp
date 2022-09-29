#include <gtest/gtest.h>

TEST(dijstra, test1)
{
    auto grid = std::vector<std::vector<int>> {
        {1, 2, 3},
        {0, 0, 0},
        {4, 5, 6}
    };
    ASSERT_TRUE(1);
}

TEST(scc, test1)
{
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}