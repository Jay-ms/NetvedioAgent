#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <stdexcept>

class ThreadPool {
public:
    // 禁止拷贝和赋值
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    explicit ThreadPool(size_t thread_count = 0);
    ~ThreadPool();

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    void worker_thread();

    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
    bool stop_flag_ = false;
};

// 构造函数
inline ThreadPool::ThreadPool(size_t thread_count) {
    if (thread_count == 0) {
        thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0) thread_count = 2; // fallback
    }
    stop_flag_ = false;
    for (size_t i = 0; i < thread_count; ++i) {
        threads_.emplace_back(&ThreadPool::worker_thread, this);
    }
}

// 析构函数
inline ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_flag_ = true;
    }
    cond_var_.notify_all();
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    // 清空任务队列
    std::queue<std::function<void()>> empty;
    std::swap(tasks_, empty);
}

inline void ThreadPool::worker_thread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_var_.wait(lock, [this] { return stop_flag_ || !tasks_.empty(); });
            if (stop_flag_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

// 模板成员函数实现必须放在头文件内
// 提交任务到线程池
// 返回 future
// 用法: auto fut = pool.submit([](int a){ return a+1; }, 5);
template <typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;
    auto task_ptr = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    std::future<return_type> res = task_ptr->get_future();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_flag_) {
            throw std::runtime_error("ThreadPool is stopped, cannot submit new tasks.");
        }
        tasks_.emplace([task_ptr]() { (*task_ptr)(); });
    }
    cond_var_.notify_one();
    return res;
}

#endif // THREAD_POOL_HPP



// #include "thread_pool.hpp"
// #include <iostream>

// int add(int a, int b) {
//     return a + b;
// }

// int main() {
//     // 创建一个线程池，自动设置线程数量为硬件核心数
//     ThreadPool pool;

//     // 提交一个带参数的任务，并获取 future
//     auto future1 = pool.submit(add, 2, 3);

//     // 提交一个 lambda 表达式任务
//     auto future2 = pool.submit([](int x) {
//         return x * x;
//     }, 6);

//     // 提交一个无参任务
//     auto future3 = pool.submit([]() {
//         std::cout << "Hello from thread pool!" << std::endl;
//         return 0;
//     });

//     // 等待任务执行完成并获取返回值
//     int result1 = future1.get(); // 5
//     int result2 = future2.get(); // 36
//     int result3 = future3.get(); // 0

//     std::cout << "add(2,3) = " << result1 << std::endl;
//     std::cout << "6 squared = " << result2 << std::endl;

//     return 0;
// }
