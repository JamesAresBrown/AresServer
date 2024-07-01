/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_CHANNEL_H
#define MYSERVER_CHANNEL_H

#include <functional>
#include <iostream>
#include <chrono>

class Socket;
class EventLoop;
class Channel {
public:
    Channel(EventLoop *loop, int fd);
    ~Channel();

    Channel(const Channel&) = delete; /* NOLINT */
    Channel& operator=(const Channel&) = delete; /* NOLINT */
    Channel(Channel&&) = delete; /* NOLINT */
    Channel& operator=(Channel&&) = delete; /* NOLINT */


    void HandleEvent();
    void EnableRead(); // 加入到epoll监听红黑树中
    //  void Read2Write();
    //  void Write2Read();
    
    int GetFd() const;
    uint32_t GetListenEvents() const;
//    uint32_t GetReadyEvents() const;
    bool GetInEpoll() const;
    void SetInEpoll(bool in = true);
    void UseET();
    void ResetListenEvents(uint32_t ev);
    
    void SetReadyEvents(uint32_t ev);
    void SetReadCallback(std::function<void()> const &callback);
    void SetWriteCallback(std::function<void()> const &callback);
    
    // private:
    EventLoop* loop_;
    int fd_;
    uint32_t listen_events_;
    uint32_t ready_events_;
    bool in_epoll_;
    std::chrono::time_point<std::chrono::system_clock> recent_active_;
    
//    std::function<void()> read_callback_ = [&](){
//        std::cout << fd_ << "no read logic" << std::endl;
//    };
      std::function<void()> read_callback_ = nullptr;
        std::function<void()> write_callback_ = [](){
        std::cout << "no write logic" << std::endl;
    };
};

#endif //MYSERVER_CHANNEL_H
