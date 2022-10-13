#pragma once

#include "observable.h"
#include "task.h"
#include "generator.h"

template <typename... Args>
class source : public observable<Args...>
{
    std::tuple<Args...> __value;
    single_consumer_event new_value_event;
public:
    explicit source(const std::tuple<Args...> &v) : __value(v) {}
    explicit source(const Args &...v) : __value(v...) {}

    template <size_t Index>
    void set(const std::remove_reference<decltype(std::get<Index>(__value))>::type &v)
    {
        std::get<Index>(__value) = v;
        this->notify_change(__value, std::index_sequence_for<Args...>{});
        new_value_event.set();
    }

    template <size_t Index>
    void set(std::remove_reference<decltype(std::get<Index>(__value))>::type &&v)
    {
        std::get<Index>(__value) = v;
        this->notify_change(__value, std::index_sequence_for<Args...>{});
        new_value_event.set();
    }

    void operator=(const std::remove_reference<decltype(std::get<0>(__value))>::type &v) requires (std::tuple_size_v<std::tuple<Args...>> == 1)
    {
        std::get<0>(__value) = v;
        this->notify_change(__value, std::index_sequence_for<Args...>{});
        new_value_event.set();
    }

    template <size_t Index>
    decltype(std::get<Index>(__value)) get() { return std::get<Index>(__value); }

    generator<std::tuple<Args...>> gen()
    {
        while (1)
        {
            co_yield __value;
        }
    }

    template <typename F>
    async<void> bind(F&& f)
    {
        auto g = this->gen();
        for(auto&& v : g)
        {
            co_await new_value_event;
            std::invoke(std::forward<F>(f), v);
            new_value_event.reset();
        }
    }

private:
    template <typename Tuple, size_t... Is>
    void notify_change(const Tuple &t, std::index_sequence<Is...>)
    {
        this->notify(std::get<Is>(t)...);
    }
};