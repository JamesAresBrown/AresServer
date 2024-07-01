/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_EVENTLOOP_H
#define MYSERVER_EVENTLOOP_H
#include "Epoll.h"

#include <functional>
#include <mutex>
#include <condition_variable>
class Epoll;
class Channel;
class EventLoop {
public:
    explicit EventLoop(int id);
    ~EventLoop();

    EventLoop(const EventLoop&) = delete; /* NOLINT */
    EventLoop& operator=(const EventLoop&) = delete; /* NOLINT */
    EventLoop(EventLoop&&) = delete; /* NOLINT */
    EventLoop& operator=(EventLoop&&) = delete; /* NOLINT */


    void Loop();
    void StopLoop();
    void UpdateChannel(Channel *ch);
    void DeleteChannel(Channel *ch);
    
    // private:
    int id_ = -1;
    std::condition_variable quit_done_condition_;
    std::mutex mutex_;
    Epoll* epoll_{nullptr};
    bool quit_{false};
};

#endif //MYSERVER_EVENTLOOP_H
