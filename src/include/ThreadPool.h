/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_THREADPOOL_H
#define MYSERVER_THREADPOOL_H
#include <condition_variable>  // NOLINT
#include <functional>
#include <future>  // NOLINT
#include <memory>
#include <mutex>  // NOLINT
#include <queue>
#include <thread>  // NOLINT
#include <utility>
#include <vector>

struct SStatus {
    bool effective;
    int connection_num;
    int activate_connection_num;
    int IO;
    int CAL;
};

class ThreadPool {
public:
    explicit ThreadPool(unsigned int size = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete; /* NOLINT */
    ThreadPool& operator=(const ThreadPool&) = delete; /* NOLINT */
    ThreadPool(ThreadPool&&) = delete; /* NOLINT */
    ThreadPool& operator=(ThreadPool&&) = delete; /* NOLINT */


    template <class F, class... Args>
    auto Add(F &&f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    int GetWorkingThreadCount() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            return workers_.size();
        }
    }
    template <class F, class... Args>
    auto Add(SStatus sStatus, F &&f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    unsigned int size_ = 1;
    unsigned int cur_size = 1;
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_variable_;
    bool stop_{false};
};

// 不能放在cpp文件，C++编译器不支持模版的分离编译
// 高度抽象任务
// Add只负责把任务放在task队列中，放在队列中的任务是一个个EventLoop，
// 每一个EventLoop负责一个客户端连接，线程池中的线程会自由竞争task
template <class F, class... Args>
auto ThreadPool::Add(F &&f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task =
        std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        // don't allow enqueueing after stopping the pool
        if (stop_) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }

        tasks_.emplace([task]() { (*task)(); });
    }
    condition_variable_.notify_one();
    return res;
}

template <class F, class... Args>
auto ThreadPool::Add(SStatus sStatus, F &&f, Args &&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    // 在加入任务前，西安动态调整线程池
    if (sStatus.effective) {
        if (sStatus.CAL < size_ && sStatus.connection_num > size_) {
            for (unsigned int i = 0; i < 2 * size_ - cur_size; ++i) {
                workers_.emplace_back([this]() {
                    while (true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(queue_mutex_);
                            // 任务队列不空 || 停止 则被唤醒
                            condition_variable_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
                            // 停止 && 没有任务 则返回
                            if (stop_ && tasks_.empty()) {
                                return;
                            }
                            task = tasks_.front(); // 取出任务
                            tasks_.pop();
                        }
                        task();
                    }
                });
            }
            cur_size = 2 * size_ - cur_size;
        }
    }


    using return_type = typename std::result_of<F(Args...)>::type;

    auto task =
            std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        // don't allow enqueueing after stopping the pool
        if (stop_) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }

        tasks_.emplace([task]() { (*task)(); });
    }
    condition_variable_.notify_one();
    return res;
}

#endif //MYSERVER_THREADPOOL_H
