#pragma once
#include <coroutine>
#include <optional>
#include <concepts>

template <std::movable T>
class generator
{
public:
    struct promise_type
    {
        generator<T> get_return_object()
        {
            return generator{Handle::from_promise(*this)};
        }
        static std::suspend_always initial_suspend() noexcept
        {
            return {};
        }
        static std::suspend_always final_suspend() noexcept
        {
            return {};
        }
        std::suspend_always yield_value(T value) noexcept
        {
            current_value = std::move(value);
            return {};
        }
        // // Allow co_await in generator coroutines.
        // template<typename U>
        // auto await_transform(U* value)
        // {
        //     struct awaiter
        //     {
        //         U* value;
        //         explicit awaiter(U* x) noexcept : value(x) {}
        //         bool await_ready() noexcept { return false; }
        //         void await_suspend(std::coroutine_handle<>) noexcept {}
        //         U* await_resume() noexcept { return value; }
        //     };
        //     return awaiter{ value };
        // }

        void unhandled_exception() noexcept
        {
            exception = std::current_exception();
        }
        void rethrow_unhandled_exception()
        {
            if (exception) std::rethrow_exception(std::move(exception));
        }
        T current_value;
        std::exception_ptr exception = nullptr;
    };

    using Handle = std::coroutine_handle<promise_type>;

    explicit generator(const Handle coroutine) : m_coroutine{coroutine}
    {
    }

    generator() = default;
    ~generator()
    {
        if (m_coroutine)
        {
            m_coroutine.destroy();
        }
    }

    generator(const generator &) = delete;
    generator &operator=(const generator &) = delete;

    generator(generator &&other) noexcept : m_coroutine{other.m_coroutine}
    {
        other.m_coroutine = {};
    }
    generator &operator=(generator &&other) noexcept
    {
        if (this != &other)
        {
            if (m_coroutine)
            {
                m_coroutine.destroy();
            }
            m_coroutine = other.m_coroutine;
            other.m_coroutine = {};
        }
        return *this;
    }

    // Range-based for loop support.
    class Iter
    {
    public:
        void operator++()
        {
            m_coroutine.resume();
        }
        const T &operator*() const
        {
            m_coroutine.promise().rethrow_unhandled_exception();
            return m_coroutine.promise().current_value;
        }
        bool operator==(std::default_sentinel_t) const
        {
            return !m_coroutine || m_coroutine.done();
        }

        explicit Iter(const Handle coroutine) : m_coroutine{coroutine}
        {
        }

    private:
        Handle m_coroutine;
    };

    Iter begin()
    {
        if (m_coroutine)
        {
            m_coroutine.resume();
        }
        return Iter{m_coroutine};
    }
    std::default_sentinel_t end()
    {
        return {};
    }

private:
    Handle m_coroutine;
};