#pragma once
#include "concurrent_queue.h"

#include <future>
#include <thread>

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
        m_tasks.emplace([task]() { (*task)();});
        m_sem.release();
        return ret;
    }
    template<typename F>
    std::future<std::invoke_result_t<F>> schedule(F&& f)
    {
        using return_type = std::invoke_result_t<F>;
        auto task = std::make_shared<std::packaged_task<return_type()>>([&]() { return f();});
        auto ret = task->get_future();
        m_tasks.emplace([task]() { (*task)();});
        m_sem.release();
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
                    auto task = std::move(m_tasks.front());
                    m_tasks.pop();
                    task();
                }
            });
            m_workers.push_back(std::move(t));
        }
    }
private:
    std::vector<std::thread> m_workers;
    concurrent_queue<std::function<void()>> m_tasks;
    std::binary_semaphore m_sem{0};
    bool m_stop{false};
};