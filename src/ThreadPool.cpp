/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#include "ThreadPool.h"

ThreadPool::ThreadPool(unsigned int size) {
    size_ = size;
    cur_size = size_;
    for (unsigned int i = 0; i < size; ++i) {
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
}
// 析构线程池的时候要等待所有任务汇合
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_variable_.notify_all();
    for (std::thread &th : workers_) {
        if (th.joinable()) {
            th.join();
        }
    }
}
