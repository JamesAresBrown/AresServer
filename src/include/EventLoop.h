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
    void StopLoopWaite();
    void UpdateChannel(Channel *ch) const;
//    void DeleteChannel(Channel *ch) const;
    
    // private:
    int id_ = -1;
    std::mutex mutex_;
    Epoll* epoll_{nullptr};
    bool quit_{false};
    std::chrono::time_point<std::chrono::system_clock> last_active_timestamp_;
    const unsigned int MAX_SEC;
};

#endif //MYSERVER_EVENTLOOP_H
