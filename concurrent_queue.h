#pragma once
#include "lock.h"

#include <queue>
#include <functional>
#include <mutex>
#include <optional>

template<typename T, typename Container = std::deque<T>>
class concurrent_queue
{
    std::queue<T, Container> queue;
    // using lock_type = std::mutex;
    using lock_type = spinlock;
    lock_type mu;

    template<typename LockType>
    class scope_lock
    {
        LockType& mu; 
        public:
            scope_lock(LockType& lock) : mu(lock) { mu.lock(); }
            ~scope_lock() { mu.unlock(); }
    };
public:
    concurrent_queue() = default;

    T& back() 
    {
        scope_lock<lock_type> l(mu);
        return queue.back();
    }

    const T& back() const
    {
        scope_lock<lock_type> l(mu);
        return queue.back();
    }

    bool empty() 
    {
        scope_lock<lock_type> l(mu);
        return queue.empty();
    }

    size_t size()
    {
        scope_lock<lock_type> l(mu);
        return queue.size();
    }

    void push(T&& value)
    {
        scope_lock<lock_type> l(mu);
        queue.push(value);
    }

    void push(const T& value)
    {
        scope_lock<lock_type> l(mu);
        queue.push(value);
    }

    void push_and_notify(T&& value, const std::function<void()>& fn)
    {
        {
            scope_lock<lock_type> l(mu);
            queue.push(value);
        }
        fn();
    }

    void push_and_notify(const T& value, const std::function<void()>& fn)
    {
        {
            scope_lock<lock_type> l(mu);
            queue.push(value);
        }
        fn();
    }

    std::optional<T> pop()
    {
        scope_lock<lock_type> l(mu);
        if (!queue.empty())
        {
            auto front = queue.front();
            queue.pop();
            return front;
        }
        else return std::nullopt;
    }

    template< class... Args >
    decltype(auto) emplace( Args&&... args )
    {
        scope_lock<lock_type> l(mu);
        queue.emplace(args...);
    }
};