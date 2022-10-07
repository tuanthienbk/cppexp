#pragma once
#include "threadpool.h"
#include <future>
#include <type_traits>
#include <coroutine>
#include <concepts>

template <std::movable T>
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
            std::cout << "return value" << std::endl;
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
            std::cout << "await_suspend" << std::endl;
            m_future.coro.resume();
            handle.resume();
        }

        T await_resume()
        {
            std::cout << "await_resume" << std::endl;
            return m_future.coro.promise().value.get();
        }

        ~awaitable() 
        { 
            std::cout << "~AwaitableFuture" << std::endl;
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

template<typename T>
struct async
{
    struct promise_type
    {
        std::coroutine_handle<> precursor;
        T value;
        std::exception_ptr exception = nullptr;

        async get_return_object() 
        {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        auto final_suspend() noexcept 
        { 
            struct awaiter 
            {
                bool await_ready() noexcept { return false; }
                void await_resume() noexcept {}
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept
                {
                    auto precursor = h.promise().precursor;
                    if (precursor)
                        return precursor;
                    else
                        return std::noop_coroutine();
                }
            };
            return awaiter{};
        }
        void return_value(T value) 
        {
            value = std::move(value);
        }
        void unhandled_exception() noexcept 
        {
            exception = std::current_exception();
        }
        void rethrow_unhandled_exception()
        {
            if (exception) std::rethrow_exception(std::move(exception));
        }
        ~promise_type() { }
    };

    std::coroutine_handle<promise_type> handle;

    bool await_ready() noexcept { return handle.done(); }
    T await_resume() noexcept 
    {
        handle.promise().rethrow_unhandled_exception();
        return handle.promise().value;
    }
    void await_suspend(std::coroutine_handle<> h) noexcept
    {
        handle.promise().precursor = h;
    }

    ~async() { }
};

template<>
struct async<void>
{
    struct promise_type
    {
        std::coroutine_handle<> precursor;
        std::exception_ptr exception = nullptr;

        async get_return_object() 
        {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        auto final_suspend() noexcept 
        { 
            struct awaiter 
            {
                bool await_ready() noexcept { return false; }
                void await_resume() noexcept {}
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept
                {
                    auto precursor = h.promise().precursor;
                    if (precursor)
                        return precursor;
                    else
                        return std::noop_coroutine();
                }
            };
            return awaiter{};
        }
        void return_void() { }
        void unhandled_exception() noexcept 
        {
            exception = std::current_exception();
        }
        void rethrow_unhandled_exception()
        {
            if (exception) std::rethrow_exception(std::move(exception));
        }
        ~promise_type() { }
    };

    std::coroutine_handle<promise_type> handle;

    bool await_ready() noexcept { return handle.done(); }
    void await_resume() noexcept 
    {
        handle.promise().rethrow_unhandled_exception();
    }
    void await_suspend(std::coroutine_handle<> h) noexcept
    {
        handle.promise().precursor = h;
    }

    ~async() { }
};