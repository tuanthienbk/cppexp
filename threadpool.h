#pragma once
#include "concurrent_queue.h"

#include <future>
#include <thread>
#include <ranges>
#include <functional>
#include <iterator>
#include <iostream>

class threadpool
{
public:
    static threadpool* instance()
    {
        static threadpool tp;
        return &tp;
    }

    ~threadpool()
    {
        m_stop = true;
        int n_threads = std::thread::hardware_concurrency();
        for(int i = 0; i < n_threads; ++i)
        {
            schedule([](){});
        }
        for(int i = 0; i < n_threads; ++i)
        {
            auto& t = m_workers[i];
            if (t.joinable()) t.join();
        }
    }
    template<typename F, typename... Args>
    std::future<std::invoke_result_t<F, Args...>> schedule(F&& f, Args&&... args)
    {
        using return_type = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args...>(args...)));
        auto ret = task->get_future();
        m_tasks.push_and_notify([task]() { (*task)();}, [this](){ m_sem.release();});
        return ret;
    }
    template<typename F>
    std::future<std::invoke_result_t<F>> schedule(F&& f)
    {
        using return_type = std::invoke_result_t<F>;
        auto task = std::make_shared<std::packaged_task<return_type()>>([&]() { return f();});
        auto ret = task->get_future();
        m_tasks.push_and_notify([task]() { (*task)();}, [this](){ m_sem.release();});
        return ret;
    }
private:
    threadpool()
    {
        int n_threads = std::thread::hardware_concurrency();
        for(int i = 0; i < n_threads; ++i)
        {
            std::thread t([this]()
            {
                while (!m_stop)
                {
                    m_sem.acquire();
                    auto task = m_tasks.pop();
                    if (task) task.value()();
                }
            });
            m_workers.push_back(std::move(t));
        }
    }
private:
    std::vector<std::thread> m_workers;
    concurrent_queue<std::function<void()>> m_tasks;
    std::counting_semaphore<32> m_sem{0};
    bool m_stop{false};
};

// template< std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
//           std::indirectly_unary_invocable<std::projected<I, Proj>> Fun >
template <typename Iterator, typename Fun>
void parallel_for(Iterator first, Iterator last, Fun f)
{
    std::vector<std::future<std::invoke_result_t<Fun, typename Iterator::value_type>>> futs;
    futs.reserve(std::distance(first, last));
    for (; first != last; ++first)
    {
        futs.push_back(threadpool::instance()->schedule(f, *first));
    }
    for(auto&& fut : futs) fut.get();
}


// template<std::ranges::input_range R, class Proj = std::identity,
//           std::indirectly_unary_invocable<std::projected<std::ranges::iterator_t<R>, Proj>> Fun >
// void parallel_for(R&& r, Fun f, Proj proj = {})
// {

// }