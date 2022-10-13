#pragma once

#include <functional>
#include <list>
#include <ranges>

template <typename ...Args>
class observable
{
    std::list<std::function<void(const Args&... )>> observers;

public:
    void notify(const Args &... value)
    {
        for (auto &&f : observers)
            std::invoke(f, value...);
    }
    void notify(Args &&... value)
    {
        for (auto &&f : observers)
            std::invoke(f, std::forward<Args...>(value...));
    }
    auto subscribe(const std::function<void(const Args & ...)> f)
    {
        return observers.insert(observers.cend(), std::move(f));
    }
    void unsubscribe(std::list<std::function<void(const Args & ...)>>::iterator cookie)
    {
        observers.erase(cookie);
    }
};

template <typename T>
class range_observable : public observable<T>
{
    // template<typename G = T>
    // using iter_type = decltype(std::declval<G>().begin());

public:
    template <typename F>
    void bind(F &&f)
    {
        T *ptr = static_cast<T *>(this);
        for (auto it = ptr->begin(); it != ptr->end(); ++it)
            std::invoke(std::forward<F>(f), *it);
    }
    void publish()
    {
        T *ptr = static_cast<T *>(this);
        for (auto it = ptr->begin(); it != ptr->end(); ++it)
            notify(*it);
    }
};