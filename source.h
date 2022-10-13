#pragma once

#include "observable.h"

template <typename... Args>
class source : public observable<Args...>
{
    std::tuple<Args...> __value;

public:
    explicit source(const std::tuple<Args...> &v) : __value(v) {}
    explicit source(const Args &...v) : __value(v...) {}

    template <size_t Index>
    void set(const std::remove_reference<decltype(std::get<Index>(__value))>::type &v)
    {
        std::get<Index>(__value) = v;
        this->notify_change(__value, std::index_sequence_for<Args...>{});
    }

    template <size_t Index>
    void set(std::remove_reference<decltype(std::get<Index>(__value))>::type &&v)
    {
        std::get<Index>(__value) = v;
        this->notify_change(__value, std::index_sequence_for<Args...>{});
    }

    void operator=(const std::remove_reference<decltype(std::get<0>(__value))>::type &v) requires (std::tuple_size_v<std::tuple<Args...>> == 1)
    {
        std::get<0>(__value) = v;
        this->notify_change(__value, std::index_sequence_for<Args...>{});
    }

    template <size_t Index>
    decltype(std::get<Index>(__value)) get() { return std::get<Index>(__value); }

private:
    template <typename Tuple, size_t... Is>
    void notify_change(const Tuple &t, std::index_sequence<Is...>)
    {
        this->notify(std::get<Is>(t)...);
    }
};