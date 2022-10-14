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
future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> task(F &&f, Args &&...args)
{
    co_return threadpool::instance()->schedule(f, args...);
}

template <typename F, typename... Args>
future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> task(launch_policy policy, F &&f, Args &&...args)
{
    if (policy == launch_policy::standard)
        co_return std::async(std::launch::async, f, args...);

    else if (policy == launch_policy::threadpool)
        co_return threadpool::instance()->schedule(f, args...);
}

template <typename T>
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
            if (exception)
                std::rethrow_exception(std::move(exception));
        }
        ~promise_type() {}
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

    ~async() {}
};

template <>
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
        void return_void() {}
        void unhandled_exception() noexcept
        {
            exception = std::current_exception();
        }
        void rethrow_unhandled_exception()
        {
            if (exception)
                std::rethrow_exception(std::move(exception));
        }
        ~promise_type() {}
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

    ~async() {}
};

/// \brief
/// A manual-reset event that supports only a single awaiting
/// coroutine at a time.
///
/// You can co_await the event to suspend the current coroutine until
/// some thread calls set(). If the event is already set then the
/// coroutine will not be suspended and will continue execution.
/// If the event was not yet set then the coroutine will be resumed
/// on the thread that calls set() within the call to set().
///
/// Callers must ensure that only one coroutine is executing a
/// co_await statement at any point in time.
class single_consumer_event
{
public:
    /// \brief
    /// Construct a new event, initialising to either 'set' or 'not set' state.
    ///
    /// \param initiallySet
    /// If true then initialises the event to the 'set' state.
    /// Otherwise, initialised the event to the 'not set' state.
    single_consumer_event(bool initiallySet = false) noexcept
        : m_state(initiallySet ? state::set : state::not_set)
    {
    }

    single_consumer_event(single_consumer_event&& other) noexcept : m_awaiter{other.m_awaiter}, m_state(other.m_state.load(std::memory_order_relaxed))
    {
        other.m_awaiter = {};
    }
    single_consumer_event& operator=(single_consumer_event&& other)
    {
        m_state = other.m_state.load(std::memory_order_relaxed);
        if (m_awaiter) m_awaiter.destroy();
        m_awaiter = other.m_awaiter;
        other.m_awaiter = {};
        return *this;
    }

    /// Query if this event has been set.
    bool is_set() const noexcept
    {
        return m_state.load(std::memory_order_acquire) == state::set;
    }

    /// \brief
    /// Transition this event to the 'set' state if it is not already set.
    ///
    /// If there was a coroutine awaiting the event then it will be resumed
    /// inside this call.
    void set()
    {
        const state oldState = m_state.exchange(state::set, std::memory_order_acq_rel);
        if (oldState == state::not_set_consumer_waiting)
        {
            m_awaiter.resume();
        }
    }

    /// \brief
    /// Transition this event to the 'non set' state if it was in the set state.
    void reset() noexcept
    {
        state oldState = state::set;
        m_state.compare_exchange_strong(oldState, state::not_set, std::memory_order_relaxed);
    }

    /// \brief
    /// Wait until the event becomes set.
    ///
    /// If the event is already set then the awaiting coroutine will not be suspended
    /// and will continue execution. If the event was not yet set then the coroutine
    /// will be suspended and will be later resumed inside a subsequent call to set()
    /// on the thread that calls set().
    auto operator co_await() noexcept
    {
        class awaiter
        {
        public:
            awaiter(single_consumer_event &event) : m_event(event) {}

            bool await_ready() const noexcept
            {
                return m_event.is_set();
            }

            bool await_suspend(std::coroutine_handle<> awaiter)
            {
                m_event.m_awaiter = awaiter;

                state oldState = state::not_set;
                return m_event.m_state.compare_exchange_strong(
                    oldState,
                    state::not_set_consumer_waiting,
                    std::memory_order_release,
                    std::memory_order_acquire);
            }

            void await_resume() noexcept {}

        private:
            single_consumer_event &m_event;
        };

        return awaiter{*this};
    }

private:
    enum class state
    {
        not_set,
        not_set_consumer_waiting,
        set
    };

    // TODO: Merge these two fields into a single std::atomic<std::uintptr_t>
    // by encoding 'not_set' as 0 (nullptr), 'set' as 1 and
    // 'not_set_consumer_waiting' as a coroutine handle pointer.
    std::atomic<state> m_state;
    std::coroutine_handle<> m_awaiter;
};