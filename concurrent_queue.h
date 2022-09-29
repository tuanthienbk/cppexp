#pragma once
#include <queue>
#include "lock.h"

template<typename T, typename Container = std::deque<T>>
class concurrent_queue
{
    std::queue<T, Container> queue;
    spinlock mu;

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

    T& front() 
    {
        return queue.front();
    }
    const T& front() const
    {
        return queue.front();
    }

    T& back() 
    {
        scope_lock l(mu);
        return queue.back();
    }

    const T& back() const
    {
        scope_lock l(mu);
        return queue.back();
    }

    bool empty() 
    {
        scope_lock l(mu);
        return queue.empty();
    }

    size_t size()
    {
        scope_lock l(mu);
        return queue.size();
    }

    void push(T&& value)
    {
        scope_lock l(mu);
        queue.push(value);
    }

    void push(const T& value)
    {
        scope_lock l(mu);
        queue.push(value);
    }

    void push_and_notify(T&& value, const std::function<void()>& fn)
    {
        {
            scope_lock l(mu);
            queue.push(value);
        }
        fn();
    }

    void push_and_notify(const T& value, const std::function<void()>& fn)
    {
        {
            scope_lock l(mu);
            queue.push(value);
        }
        fn();
    }

    void pop()
    {
        scope_lock<spinlock> l(mu);
        queue.pop();
    }

    template< class... Args >
    decltype(auto) emplace( Args&&... args )
    {
        scope_lock<spinlock> l(mu);
        queue.emplace(args...);
    }
};