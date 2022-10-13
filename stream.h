#pragma once

#include "generator.h" 
#include "task.h"

#include <random>
#include <string>
#include <fstream>

template <std::movable T>
struct stream_base
{
    virtual void start() = 0;
    virtual generator<T> get() = 0;
    virtual void stop() = 0;
};

template <std::movable T>
class random_number_stream : public stream_base<T> 
{
    std::uniform_real_distribution<T> m_distrib;
    std::mt19937 m_gen;
    bool m_stop;
public:
    random_number_stream(T low, T high) : m_stop(true)
    {
        m_distrib = std::move(std::uniform_real_distribution<T>(low,high));
        std::random_device rd;
        m_gen = std::move(std::mt19937(rd()));
    }
    void start() override
    {
        m_stop = false;
    }
    void stop() override
    {
        m_stop = true;
        m_distrib.reset();
    }
    generator<T> get() override
    {
        while (1)
        {
            if (!m_stop)
                co_yield m_distrib(m_gen);
            else
                throw "generator is stopped ?";
        }
    }
};

template <>
class random_number_stream<int> : public stream_base<int>
{
    std::uniform_int_distribution<> m_distrib;
    std::mt19937 m_gen;
    bool m_stop;
public:
    random_number_stream(int low, int high) : m_stop(true)
    {
        m_distrib = std::move(std::uniform_int_distribution<>(low,high));
        std::random_device rd;
        m_gen = std::move(std::mt19937(rd()));
    }
    void start() override
    {
        m_stop = false;
    }
    void stop() override
    {
        m_stop = true;
        m_distrib.reset();
    }
    generator<int> get() override
    {
        while (true) 
        {
            if (!m_stop)
                co_yield m_distrib(m_gen);
            else
                throw "generator is stopped ?";
        }
    }
};

template<std::movable T>
struct async_stream : public stream_base<T>
{
    async_stream() {}
    async_stream(const T& v) : value(v) {}

    virtual bool is_stopped() = 0;

    struct promise_value
    {
        promise_value() {}
        promise_value(const T& v) : value(v) {}

        T value;
        std::binary_semaphore sem{0};
        void set(T&& v)
        {
            // std::cout << "set" << std::endl;
            value = std::move(v);
            sem.release();
        }
        T get()
        {
            // std::cout << "get " << this << std::endl;
            sem.acquire();
            return value;
        }
    };

    struct future_from_stream
    {
        struct promise_type
        {
            promise_value* value;
            future_from_stream get_return_object()
            {
                // std::cout << "get_return_object" << std::endl;
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
            void return_value(promise_value* x)
            {
                // std::cout << "return value " << x << std::endl;
                value = x;
            }
            void unhandled_exception() noexcept {}

            ~promise_type() 
            { 
                // std::cout << "future ~promise_type" << std::endl;
            }
        };
        
        std::coroutine_handle<promise_type> handle;
        bool await_ready() const noexcept 
        { 
            // std::cout << "await_ready" << std::endl;
            return handle.done();
        }
        void await_suspend(std::coroutine_handle<> h)
        {
            // std::cout << "await_suspend" << std::endl;
            // the order is important, when handle resume, the return_value in the promise type is call
            handle.resume();
            h.resume();
        }
        T await_resume()
        {
            // std::cout << "await_resume" << std::endl;
            // the value is already set in return_value, otherwise it will be crashed !!
            return handle.promise().value->get();
        }
    };

    promise_value value;

    future_from_stream next() { co_return std::addressof(value); } 

    generator<T> get() override
    {
        // std::cout << std::addressof(value) << std::endl;
        while (true)
        {
            if (!is_stopped())
            {
                auto v = co_await next();
                co_yield v;
            }
            else
            {
                throw "stream is stopped ?";
            }
        }
    }
};

class text_file_stream : public async_stream<std::string>
{
    std::thread t;
    std::atomic<bool> isStopped;
public:
    text_file_stream(const char* filename) 
        : isStopped(false),
        t(std::move(std::thread([=,this]()
        {
            // std::cout << "go in thread" << std::endl;
            std::ifstream fs(filename, std::ios::in);
            if (fs.is_open())
            {
                // std::cout << "file is open" << std::endl;
                while (!isStopped)
                {
                    // using namespace std::literals;
                    // std::this_thread::sleep_for(200ms);
                    std::string line; 
                    if (std::getline(fs, line))
                    {
                        // std::cout << "line :" << line << std::endl;
                        value.set(std::move(line));
                    }
                    else break;
                }
                isStopped = true;
            }
            else
            {
                std::cout << "file not found !" << std::endl;
            }
        })))
    {}

    void start() override { isStopped = false; }
    void stop() override { isStopped = true; if (t.joinable()) t.join(); } 
    bool is_stopped() override { return isStopped; }
};