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
    // ��ֹ�����͸�ֵ
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

// ���캯��
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

// ��������
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
    // ����������
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

// ģ���Ա����ʵ�ֱ������ͷ�ļ���
// �ύ�����̳߳�
// ���� future
// �÷�: auto fut = pool.submit([](int a){ return a+1; }, 5);
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
//     // ����һ���̳߳أ��Զ������߳�����ΪӲ��������
//     ThreadPool pool;

//     // �ύһ�������������񣬲���ȡ future
//     auto future1 = pool.submit(add, 2, 3);

//     // �ύһ�� lambda ���ʽ����
//     auto future2 = pool.submit([](int x) {
//         return x * x;
//     }, 6);

//     // �ύһ���޲�����
//     auto future3 = pool.submit([]() {
//         std::cout << "Hello from thread pool!" << std::endl;
//         return 0;
//     });

//     // �ȴ�����ִ����ɲ���ȡ����ֵ
//     int result1 = future1.get(); // 5
//     int result2 = future2.get(); // 36
//     int result3 = future3.get(); // 0

//     std::cout << "add(2,3) = " << result1 << std::endl;
//     std::cout << "6 squared = " << result2 << std::endl;

//     return 0;
// }
