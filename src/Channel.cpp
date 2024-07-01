/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#include "Channel.h"
#include <sys/epoll.h>
#include "EventLoop.h"

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), listen_events_(0), ready_events_(0), in_epoll_(false), recent_active_(std::chrono::system_clock::now()) {}

Channel::~Channel() {
    // 成员变量loop_并不由Channel管理
    // 这个fd_和绑定的socket是一样的，通过socket关闭
//    if (fd_ != -1) {
//        close(fd_);
//        fd_ = -1;
//    }
}

void Channel::HandleEvent() {
    if (ready_events_ & (EPOLLIN | EPOLLPRI)) {
        read_callback_();
    }

    if (ready_events_ & (EPOLLOUT)) {
        recent_active_ = std::chrono::system_clock::now();
        write_callback_();
    }
}

void Channel::EnableRead() { // 加入到epoll监听红黑树中
    listen_events_ |= EPOLLIN | EPOLLPRI;
    loop_->UpdateChannel(this); // eventloop维护epoll对象，所以加入事件由eventloop负责
}
//void Channel::Read2Write() { // 加入到epoll监听红黑树中
//    listen_events_ = EPOLLOUT| EPOLLRDHUP;
//    loop_->UpdateChannel(this); // eventloop维护epoll对象，所以加入事件由eventloop负责
//}
//void Channel::Write2Read() { // 加入到epoll监听红黑树中
//    listen_events_ = EPOLLIN | EPOLLPRI;
//    loop_->UpdateChannel(this); // eventloop维护epoll对象，所以加入事件由eventloop负责
//}

void Channel::UseET() {
    listen_events_ |= EPOLLET;
    loop_->UpdateChannel(this);
}
int Channel::GetFd() const { return fd_; }

uint32_t Channel::GetListenEvents() const { return listen_events_; }
//uint32_t Channel::GetReadyEvents() const { return ready_events_; }

bool Channel::GetInEpoll() const { return in_epoll_; }

void Channel::SetInEpoll(bool in) { in_epoll_ = in; }

void Channel::ResetListenEvents(uint32_t ev) {
  listen_events_ = ev;
  loop_->UpdateChannel(this);
}

void Channel::SetReadyEvents(uint32_t ev) { ready_events_ = ev; }

void Channel::SetReadCallback(std::function<void()> const &callback) { read_callback_ = callback; }

void Channel::SetWriteCallback(std::function<void()> const &callback) { write_callback_ = callback; }