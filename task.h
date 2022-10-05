#pragma once
#include "threadpool.h"
#include <future>
#include <type_traits>
#include <coroutine>

template <typename T>
struct future
{
    struct promise_type
    {
        std::future<T> value;
        future get_return_object()
        {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept
        {
            // std::cout << "initial" << std::endl;
            return {};
        }
        std::suspend_always final_suspend() noexcept
        {
            // std::cout << "final" << std::endl;
            return {};
        }
        void return_value(std::future<T> x)
        {
            // std::cout << "return value" << std::endl;
            value = std::move(x);
        }
        void unhandled_exception() noexcept {}

        ~promise_type() 
        { 
            // std::cout << "future ~promise_type" << std::endl;
        }
    };

    struct awaitable
    {
        future &m_future;
        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> handle)
        {
            // std::cout << "await_suspend" << std::endl;
            m_future.coro.resume();
            handle.resume();
        }

        T await_resume()
        {
            // std::cout << "await_resume" << std::endl;
            return m_future.coro.promise().value.get();
        }

        ~awaitable() 
        { 
            // std::cout << "~AwaitableFuture" << std::endl;
        }
    };

    std::coroutine_handle<promise_type> coro;

    future(std::coroutine_handle<promise_type> coro) : coro{coro} {}

    ~future()
    {
        // std::cout << "~future" << std::endl;
        if (coro)
            coro.destroy();
    }

    awaitable operator co_await()
    {
        // std::cout << "co_await" << std::endl;
        return {*this};
    }
};

enum class launch_policy
{
    standard,
    threadpool
};

template <typename F, typename... Args>
future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> task(F&& f, Args&&... args)
{
    co_return threadpool::instance()->schedule(f, args...);
}

template <typename F, typename... Args>
future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> task(launch_policy policy, F&& f, Args&&... args)
{
    if (policy == launch_policy::standard)
        co_return std::async(std::launch::async, f, args...);
    
    else if (policy == launch_policy::threadpool)
        co_return threadpool::instance()->schedule(f, args...);
}

struct async
{
    struct promise_type
    {
        async get_return_object() { return {}; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() noexcept {}
        ~promise_type() { }
    };

    ~async() { }
};

#define UNIT_TEST

#if defined(UNIT_TEST)

async test1()
{
    using namespace std::literals;
    auto result  = co_await task([]() -> int 
    { 
        std::this_thread::sleep_for(100ms); 
        return 1; 
    });
}
#endif
