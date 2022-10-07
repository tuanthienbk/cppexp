#include "stream.h"

#include <gtest/gtest.h>

TEST(stream, test1)
{
    random_number_stream<float> random(0.0f, 1.0f);
    random.start();
    int i = 0;
    try
    {
        for(auto&& number : random.get())
        {
            std::cout << "i = " << i++ << ": " << number << std::endl;
            if (i == 5) random.stop();
            if (i == 10) break; 
        }
    }
    catch (std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }
    random.stop();
}

TEST(stream, test2)
{
    random_number_stream<int> random(0, 10);
    random.start();
    int i = 0;
    try
    {
        for(auto&& number : random.get())
        {
            std::cout << "i = " << i++ << ": " << number << std::endl;
            // if (i == 5) random.start();
            if (i == 10) break; 
        }
    }
    catch (std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }
    random.stop();
}

TEST(stream, test3)
{
    text_file_stream fs("CMakeCache.txt");
    fs.start();
    try
    {
        for(auto&& line : fs.get())
            std::cout << line << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    fs.stop();
}