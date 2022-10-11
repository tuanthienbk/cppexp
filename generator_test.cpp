#include "generator.h"

#include <ranges>
#include <gtest/gtest.h>

generator<int> range(int low, int high)
{
    for(int i = low; i < high; ++i)
        co_yield i;
}

struct stringview_sentinel
{
    bool operator==(const char* p) const
    {
        return *p == '\0';
    }
};

class StringView //: public std::ranges::view_interface<StringView> 
{
public:
    StringView() = default;
    StringView(const char* string) : ptr(string){}
    auto begin() const { return ptr;}
    auto end() const { return stringview_sentinel{};}
private:
    const char* ptr;
};

struct Iter
{
    // typedef Iter                                                      iterator_type;
    typedef  int        value_type;
    typedef  std::ptrdiff_t difference_type;
    // typedef  const int* pointer;
    // typedef  const int& reference;
    // typedef  std::input_iterator_tag iterator_category;
    int* p;
    Iter(int* ptr) : p(ptr) {}
    Iter& operator++() noexcept { p++; return *this;}
    // Iter& operator--() noexcept { p++; return *this;}
    Iter operator++(int) noexcept { auto temp = *this; p++; return temp;} 
    // Iter operator--(int) noexcept { auto temp = *this; p++; return temp;} 
    const int &operator*() const noexcept { return *p; }
    // const int* operator->() const noexcept { return p; }
};

// bool operator==(const Iter& iter1, const Iter& iter2) { return iter1.p == iter2.p;}
// bool operator!=(const Iter& iter1, const Iter& iter2) { return iter1.p != iter2.p;}
// bool operator<(const Iter& iter1, const Iter& iter2) { return iter1.p < iter2.p;}
// bool operator<=(const Iter& iter1, const Iter& iter2) { return iter1.p <= iter2.p;}
// bool operator>=(const Iter& iter1, const Iter& iter2) { return iter1.p >= iter2.p;}

struct zstring : std::ranges::view_interface<zstring> {
    int* p = nullptr;
    zstring() = default;
    zstring(int* p) : p(p) { }

    struct zstring_sentinel {
        bool operator==(const Iter& iter) const {
            return *iter.p == 0;
        }
        // bool operator==(const int* iter) const {
        //     return *iter == 0;
        // }
    };
    auto begin() const { return Iter{p}; }
    auto end() const { return zstring_sentinel{}; }
    // auto begin() const { return p; }
    // auto end() const { return zstring_sentinel{}; }
};

TEST(generator, test1)
{
    for(auto& i : range(10, 20))
        std::cout << i << std::endl;
}

TEST(generator, test2)
{
    // for (int year : std::views::iota(2017)
    //               | std::views::take_while([](int y) { return y <= 2020; })) {
    //     std::cout << year << ' ';
    // }

    for(auto& i : range(10, 20) | std::views::filter([](int i){ return 0 == i % 2;}))
        std::cout << i << std::endl;

    // std::vector<int> v = {1, 2, 3, 4, 5};
    // const auto it = v.cbegin();

    // // const char* str = "Hello World!";
    // int numbers[] = { 1, 2, 3, 4, 5, 0};
    // zstring view_over_v{numbers};
    // // for(auto&& c : view_over_v)
    // //     std::cout << c << std::endl;
    // for(auto&& i : view_over_v | std::views::filter([](int i){ return 0 == i % 2;}) | std::views::transform([](int i) { return i * i;}))
    // // for(auto&& i : view_over_v | std::views::filter([](char c){ return c != 'o';}) )
    //     std::cout << i << std::endl;
}