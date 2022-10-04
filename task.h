#pragma once
#include "threadpool.h"
#include <future>
#include <type_traits>
#include <coroutine>

template<class T>
class task
{
    std::binary_semaphore m_sem{0};
    T m_value;
    std::function<void()> m_func;
public:
    template<class Fp, class ...Args > requires(std::is_same<T, std::invoke_result_t<Fp, Args...>>::value)
    task(Fp&& f, Args&&... args)
    {
        m_func = [=, this]()
        {
            m_value = f(args...);
            m_sem.release();
        };
    }
    struct awaitable
    {
        task* t;
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> h) 
        {
            threadpool::instance()->enqueue(t->m_func);
            h.resume();
        }
        T await_resume() 
        {
            t->m_sem.acquire();
            return m_value;
        }
    };

    awaitable operator co_await()
    {
        return {this};
    }
};

template<>
class task<void>
{
    std::binary_semaphore m_sem{0};
    std::function<void()> m_func;
public:
    template<class Fp, class ...Args > requires(std::is_same<void, std::invoke_result_t<Fp, Args...>>::value)
    task(Fp&& f, Args&&... args)
    {
        m_func = [=, this]()
        {
           f(args...); 
           m_sem.release();
        };
    }
    struct awaitable
    {
        task* t;
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> h) 
        {
            threadpool::instance()->enqueue(t->m_func);
            h.resume();
        }
        void await_resume() 
        {
            t->m_sem.acquire();
        }
    };

    awaitable operator co_await()
    {
        return {this};
    }

};

template <typename T> struct future {
  struct promise_type {
    std::future<T> value;
    future get_return_object() {
      return {std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() noexcept {
      std::cout << "initial" << std::endl;
      return {};
    }
    std::suspend_always final_suspend() noexcept {
      std::cout << "final" << std::endl;
      return {};
    }
    void return_value(std::future<T> x) {
      std::cout << "return value" << std::endl;
      value = std::move(x);
    }
    void unhandled_exception() noexcept {}

    ~promise_type() { std::cout << "future ~promise_type" << std::endl; }
  };

  struct AwaitableFuture {
    future &m_future;
    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
      std::cout << "await_suspend" << std::endl;
        m_future.coro.resume();
        handle.resume();
    }

    T await_resume() {
      std::cout << "await_resume" << std::endl;
      return m_future.coro.promise().value.get();
    }

    ~AwaitableFuture() { std::cout << "~AwaitableFuture" << std::endl; }
  };

  std::coroutine_handle<promise_type> coro;

  future(std::coroutine_handle<promise_type> coro) : coro{coro} {}

  ~future() {
    std::cout << "~future" << std::endl;
    if (coro)
      coro.destroy();
  }

  AwaitableFuture operator co_await() {
    std::cout << "co_await" << std::endl;
    return {*this};
  }
};

template <typename F, typename... Args>
future<std::invoke_result_t<F, Args...>> async(F f, Args... args) {
  std::cout << "async" << std::endl;
  co_return threadpool::instance()->schedule(f, args...);
}

struct async_task {

  struct promise_type {
    async_task get_return_object() { return {}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() noexcept {}
    ~promise_type() { std::cout << "~task promise_type" << std::endl; }
  };

  ~async_task() { std::cout << "~task" << std::endl; }
};

int square(int x) {
  std::cout << "square in thread id " << std::this_thread::get_id()
            << std::endl;
  return x * x;
}

async_task f() {
  auto squared6 = co_await async(square, 6);

  std::cout << "Write " << squared6
            << " from thread: " << std::this_thread::get_id() << std::endl;
}