/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_ACCEPTOR_H
#define MYSERVER_ACCEPTOR_H
#include <functional>

/**
 * 类存在于事件驱动EventLoop类中，也就是Reactor模式的main-Reactor
 * 类中的socket fd就是服务器监听的socket fd，每一个Acceptor对应一个socket fd
 * 这个类也通过一个独有的Channel负责分发到epoll，该Channel的事件处理函数handleEvent()会调用Acceptor中的接受连接函数来新建一个TCP连接
 */
// 专门处理连接事件
class EventLoop;
class Socket;
class Channel;
class Acceptor {
public:
    explicit Acceptor(EventLoop *loop);
    ~Acceptor();

    Acceptor(const Acceptor&) = delete; /* NOLINT */
    Acceptor& operator=(const Acceptor&) = delete; /* NOLINT */
    Acceptor(Acceptor&&) = delete; /* NOLINT */
    Acceptor& operator=(Acceptor&&) = delete; /* NOLINT */

    void AcceptConnection();
    void SetNewConnectionCallback(std::function<void(Socket *)> const &callback);

private:
    EventLoop* loop_;
    Socket* sock_;
    Channel* channel_;
    std::function<void(Socket*)> new_connection_callback_; // 定义一个新建连接的回调函数
};

#endif //MYSERVER_ACCEPTOR_H
