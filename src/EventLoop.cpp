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
// EventLoop的操作实际上是对epoll进行各种操作，并提供事件循环
// 创建epoll
// epoll对象似乎只由EventLoop对象管理，类acceptor， channel，connect通过绑定该EventLoop来进行epoll相关操作
EventLoop::EventLoop(int id) : id_(id), MAX_SEC(0) {
    epoll_ = new Epoll();
    last_active_timestamp_ = std::chrono::system_clock::now();
} // 核心操作是epoll_create1

EventLoop::~EventLoop() {
    delete epoll_;
}

void EventLoop::Loop() {
    bool stop = false;
    while (!stop) {
//        std::vector<Channel *> chs;
        // 核心操作是epoll_wait
        std::vector<Channel *> chs = epoll_->Poll(1);
        for (auto &ch : chs) {
            ch->HandleEvent();
            last_active_timestamp_ = std::chrono::system_clock::now();
        }
        { // 加锁关闭操作
            std::unique_lock<std::mutex> lock(mutex_);
            if (quit_) {
                stop = true;
                printf("Loop %d stop.\n", id_);
            }
        }
        auto duration = std::chrono::system_clock::now() - last_active_timestamp_;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        if (MAX_SEC > 0 && seconds > MAX_SEC) {
            { // 加锁关闭操作
                std::unique_lock<std::mutex> lock(mutex_);
                quit_ = true;
            }
            stop = true;
            printf("do nothing > %d sec, Loop %d stop.\n", MAX_SEC, id_);
        }
    }
}

void EventLoop::UpdateChannel(Channel *ch) const {
    epoll_->UpdateChannel(ch);
} // 核心操作是epoll_clt

//void EventLoop::DeleteChannel(Channel *ch) const {
//    epoll_->DeleteChannel(ch);
//}

void EventLoop::StopLoopWaite() {
    std::unique_lock<std::mutex> lock(mutex_);
    quit_ = true;
}