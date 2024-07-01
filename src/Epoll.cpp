/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#include "Epoll.h"
#include <unistd.h>
#include <cstring>
#include <sys/epoll.h>
#include "Channel.h"
#include "util.h"

#define MAX_EVENTS 1000

Epoll::Epoll() {
    epfd_ = epoll_create1(0);
    ErrorIf(epfd_ == -1, "epoll create error");
    // 设置非阻塞
//    fcntl(epfd_, F_SETFL, fcntl(epfd_, F_GETFL, 0) | O_NONBLOCK);
    events_ = new epoll_event[MAX_EVENTS];
    memset(events_, 0, sizeof(*events_) * MAX_EVENTS);
}

Epoll::~Epoll() {
    if (epfd_ != -1) {
        close(epfd_);
        epfd_ = -1;
    }
    delete[] events_;
}

std::vector<Channel *> Epoll::Poll(int timeout) {
    std::vector<Channel* > active_channels;
    int nfds = epoll_wait(epfd_, events_, MAX_EVENTS, timeout);
    if (errno == EINTR) {
        // 如果系统调用被信号中断，重新等待事件
        return {};
    } else {
        ErrorIf(nfds == -1, "epoll wait error");
    }
    for (int i = 0; i < nfds; ++i) {
        Channel* ch;
        ch = (Channel *) events_[i].data.ptr;
        ch->SetReadyEvents(events_[i].events);
        active_channels.push_back(ch);
    }
    return active_channels;
}

void Epoll::UpdateChannel(Channel *ch) { // epoll_ctl 并且将channel与该event绑定
    int fd = ch->GetFd();
    struct epoll_event ev {};
    ev.data.ptr = ch;
    ev.events = ch->GetListenEvents();
    if (!ch->GetInEpoll() && fd >= 0) {
        ErrorIf(epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1, "epoll add error");
        ch->SetInEpoll();
    } else {
        ErrorIf(epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == -1, "epoll modify error");
    }
}

void Epoll::DeleteChannel(Channel *ch) {
    int fd = ch->GetFd();
    if (ch->GetInEpoll() && fd >= 0) {
        ErrorIf(epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr) == -1, "epoll delete error");
    }
    ch->SetInEpoll(false);
}
