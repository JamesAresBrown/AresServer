/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#include "EventLoop.h"
#include <vector>
#include <mutex>
#include "Channel.h"
#include "Epoll.h"
// eventloop的操作实际上是对epoll进行各种操作，并提供事件循环
// 创建epoll
// epoll对象似乎只由eventloop对象管理，类acceptor， channel，connect通过绑定该eventloop来进行epoll相关操作
EventLoop::EventLoop(int id) : id_(id) {
    epoll_ = new Epoll();
} // 核心操作是epoll_create1

EventLoop::~EventLoop() {
    delete epoll_;
}

void EventLoop::Loop() {
    bool stop = false;
    while (!stop) {
        std::vector<Channel *> chs;
        chs = epoll_->Poll(); // 核心操作是epoll_wait
        for (auto &ch : chs) {
            ch->HandleEvent();
        }
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (quit_) {
//                quit_done_ = true;
                stop = true;
                quit_done_condition_.notify_one();
                lock.unlock();
            }
        }
    }
}

void EventLoop::UpdateChannel(Channel *ch) {
    epoll_->UpdateChannel(ch);
} // 核心操作是epoll_clt

void EventLoop::DeleteChannel(Channel *ch) {
    epoll_->DeleteChannel(ch);
}

void EventLoop::StopLoop() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        quit_ = true;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    quit_done_condition_.wait(lock, [&](){return true;});
    printf("reactor %d stop.\n", id_);
}