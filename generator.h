#pragma once

#include "observable.h"

#include <coroutine>
#include <optional>
#include <concepts>
#include <ranges>
#include <functional>
#include <iostream>

template <std::movable T>
class generator : public range_observable<generator<T>>
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
        void unhandled_exception() noexcept
        {
            exception = std::current_exception();
        }
        void rethrow_unhandled_exception()
        {
            if (exception)
                std::rethrow_exception(std::move(exception));
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
        typedef T value_type;
        typedef std::ptrdiff_t difference_type;
        typedef const T &reference;
        typedef const T *pointer;
        typedef std::input_iterator_tag iterator_category;

        Iter& operator++() noexcept
        {
            // std::cout << "generator ++" << std::endl;
            m_coroutine.resume();
            return *this;
        }
        Iter operator++(int) noexcept
        {
            Iter temp = *this;
            m_coroutine.resume();
            return temp;
        }
        const T &operator*() const noexcept
        {
            m_coroutine.promise().rethrow_unhandled_exception();
            const T& value = m_coroutine.promise().current_value;
            return value;
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

template <std::ranges::input_range R> // requires std::ranges::view<R>
class custom_take_view : public std::ranges::view_interface<custom_take_view<R>>, 
    public range_observable<custom_take_view<R>>
{
    struct Iter
    {
        typedef typename std::ranges::iterator_t<R>::value_type value_type;
        typedef std::ptrdiff_t difference_type;
        typedef const value_type& reference;
        typedef const value_type* pointer;
        typedef std::input_iterator_tag iterator_category;

        std::ranges::iterator_t<R> __current;
        std::iter_difference_t<std::ranges::iterator_t<R>> __count{};
        std::iter_difference_t<std::ranges::iterator_t<R>> __index{};

        Iter(const std::ranges::iterator_t<R> &_begin, std::iter_difference_t<std::ranges::iterator_t<R>> _count) : __current(_begin), __count(_count) {}

        reference operator*() const noexcept 
        { 
            // std::cout<< "custom take *" << std::endl;

            return *__current; 
        }

        Iter& operator++() noexcept
        {
            // std::cout<< "custom take ++()" << std::endl;
            ++__current;
            __index++;
            return *this;
        }
        Iter operator++(int) noexcept
        {
            // std::cout<< "custom take ++(int)" << std::endl;
            Iter temp = *this;
            ++__current;
            __index++;
            return temp;
        }
        bool operator==(std::default_sentinel_t sentinel) const
        {
            // std::cout<< "custom take == sentinel" << std::endl;
            return (__current == sentinel) || (__index == __count);
        }
    };

private:
    R base_;
    std::iter_difference_t<std::ranges::iterator_t<R>> count_;
public:
    custom_take_view() = default;

    constexpr custom_take_view(R base, std::iter_difference_t<std::ranges::iterator_t<R>> count)
        : base_(std::move(base)), count_(count) {}

    constexpr auto begin() 
    {
        return Iter(std::begin(base_), count_);
    }
    constexpr auto end() 
    {
        return std::default_sentinel_t{};
    }
};

template <std::ranges::input_range R>
custom_take_view(R &&base, std::iter_difference_t<std::ranges::iterator_t<R>>)
    -> custom_take_view<std::ranges::views::all_t<R>>;

namespace details
{
    struct custom_take_range_adaptor_closure
    {
        std::size_t count_;
        constexpr custom_take_range_adaptor_closure(std::size_t count) : count_(count)
        {
        }

        template <std::ranges::input_range R>
        constexpr auto operator()(R &&r) const
        {
            return custom_take_view(std::forward<R>(r), count_);
        }
    };

    struct custom_take_range_adaptor
    {
        template <std::ranges::input_range R>
        constexpr auto operator()(R &&r, std::iter_difference_t<std::ranges::iterator_t<R>> count)
        {
            return custom_take_view(std::forward<R>(r), count);
        }

        constexpr auto operator()(std::size_t count)
        {
            return custom_take_range_adaptor_closure(count);
        }
    };

    template <std::ranges::input_range R>
    constexpr auto operator|(R &&r, custom_take_range_adaptor_closure const &a)
    {
        return a(std::forward<R>(r));
    }
}

namespace views
{
    static details::custom_take_range_adaptor take;
}

template <std::ranges::input_range R, std::copy_constructible F> // requires std::ranges::view<R>
class custom_transform_view : public std::ranges::view_interface<custom_transform_view<R, F>>,
    public range_observable<custom_transform_view<R,F>>
{
    struct Iter
    {
        typedef std::invoke_result_t<F, typename std::ranges::iterator_t<R>::value_type> value_type;
        typedef std::ptrdiff_t difference_type;
        typedef const value_type &reference;
        typedef const value_type *pointer;
        typedef std::input_iterator_tag iterator_category;

        std::ranges::iterator_t<R> __current;
        F __func;
        value_type __value;

        Iter(const std::ranges::iterator_t<R> &_begin, F f) 
        : __current(_begin), 
        __func(std::move(f)),
        __value(std::invoke(__func,*__current))
        {}

        reference operator*() const noexcept 
        { 
            // std::cout<< "custom transform *" << std::endl;
            return __value;
        }
        Iter& operator++() noexcept
        {
            // std::cout<< "custom transform ++()" << std::endl;
            ++__current;
            __value = std::invoke(__func,*__current);
            return *this;
        }
        Iter operator++(int) noexcept
        {
            // std::cout<< "custom transform ++(int)" << std::endl;
            Iter temp = *this;
            ++__current;
            __value = std::invoke(__func,*__current);
            return temp;
        }
        bool operator==(std::default_sentinel_t sentinel) const
        {
            // std::cout<< "custom transform ==" << std::endl;
            return __current == sentinel;
        }
    };

private:
    R base_;
    F func_;
public:
    custom_transform_view() = default;

    constexpr custom_transform_view(R base, F f)
        : base_(std::move(base)), func_(std::move(f)) {}

    constexpr auto begin() 
    {
        return Iter(std::begin(base_), func_);
    }
    constexpr auto end() 
    {
        return std::default_sentinel_t{};
    }
};

namespace details
{
    template<typename F>
    struct custom_transform_range_adaptor_closure
    {
        F __f;
        explicit custom_transform_range_adaptor_closure(F f) : __f(std::move(f)) {}
        template <std::ranges::input_range R>
        constexpr auto operator()(R &&r) const
        {
            return custom_transform_view(std::forward<R>(r), __f);
        }
    };

    struct custom_transform_range_adaptor
    {
        template<typename F>
        constexpr auto operator()(F&& f) const
        {
            return custom_transform_range_adaptor_closure(std::move(f));
        }
    };

    template <std::ranges::input_range R, typename F>
    constexpr auto operator|(R &&r, custom_transform_range_adaptor_closure<F> const &a)
    {
        return a(std::forward<R>(r));
    }
}

namespace views
{
    static details::custom_transform_range_adaptor transform;
}

template <std::ranges::input_range R, typename Pred> 
requires std::predicate<Pred, const typename std::ranges::iterator_t<R>::value_type&>
class custom_filter_view : public std::ranges::view_interface<custom_filter_view<R, Pred>>,
    public range_observable<custom_filter_view<R,Pred>>
{
    struct Iter
    {
        typedef typename std::ranges::iterator_t<R>::value_type value_type;
        typedef std::ptrdiff_t difference_type;
        typedef const value_type &reference;
        typedef const value_type *pointer;
        typedef std::input_iterator_tag iterator_category;

        std::ranges::iterator_t<R> __current;
        Pred __func;

        Iter(const std::ranges::iterator_t<R> &_begin, Pred f) 
        : __current(_begin), 
        __func(std::move(f))
        {}

        reference operator*() const noexcept 
        { 
            // std::cout<< "custom transform *" << std::endl;
            return *__current;
        }
        Iter& operator++() noexcept
        {
            // std::cout<< "custom transform ++()" << std::endl;
            std::default_sentinel_t sentinel;
            do
            {
                ++__current;
            } while (!(__current == sentinel) && !std::invoke(__func,*__current));
            return *this;
        }
        Iter operator++(int) noexcept
        {
            // std::cout<< "custom transform ++(int)" << std::endl;
            Iter temp = *this;
            std::default_sentinel_t sentinel;
            do
            {
                ++__current;
            } while (!(__current == sentinel) && !std::invoke(__func,*__current));
            return temp;
        }
        bool operator==(std::default_sentinel_t sentinel) const
        {
            // std::cout<< "custom transform ==" << std::endl;
            return __current == sentinel;
        }
    };

private:
    R base_;
    Pred func_;
public:
    custom_filter_view() = default;

    constexpr custom_filter_view(R base, Pred f)
        : base_(std::move(base)), func_(std::move(f)) {}

    constexpr auto begin() 
    {
        return Iter(std::begin(base_), func_);
    }
    constexpr auto end() 
    {
        return std::default_sentinel_t{};
    }
};

namespace details
{
    template<typename Pred>
    struct custom_filter_range_adaptor_closure
    {
        Pred __f;
        explicit custom_filter_range_adaptor_closure(Pred f) : __f(std::move(f)) {}
        template <std::ranges::input_range R>
        constexpr auto operator()(R &&r) const
        {
            return custom_filter_view(std::forward<R>(r), __f);
        }
    };

    struct custom_filter_range_adaptor
    {
        template<typename Pred>
        constexpr auto operator()(Pred&& f) const
        {
            return custom_filter_range_adaptor_closure(std::move(f));
        }
    };

    template <std::ranges::input_range R, typename Pred>
    constexpr auto operator|(R &&r, custom_filter_range_adaptor_closure<Pred> const &a)
    {
        return a(std::forward<R>(r));
    }
}

namespace views
{
    static details::custom_filter_range_adaptor filter;
}