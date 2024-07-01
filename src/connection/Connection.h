/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_CONNECTION_H
#define MYSERVER_CONNECTION_H

#include "Channel.h"
#include "Application.h"
#include <functional>

class EventLoop;
class Socket;
class Channel;
class Connection {
public:
    Application* application_{nullptr};
//    enum State {
//        Invalid = 1,
//        Handshaking,
//        Connected,
//        Closed,
//        Failed,
//    };
    Connection(EventLoop *loop, Socket *sock);
    ~Connection();
    Connection(const Connection&) = delete; /* NOLINT */
    Connection& operator=(const Connection&) = delete; /* NOLINT */
    Connection(Connection&&) = delete; /* NOLINT */
    Connection& operator=(Connection&&) = delete; /* NOLINT */

    void EnableRead();
    void SetDeleteConnectionCallback(std::function<void(Socket*)> const& callback);
    //  void SetOnConnectCallback(std::function<void(Connection *)> const &callback);
    void SetReadConnectCallback(std::function<void(Connection*)> const& callback);
    void SetWriteConnectCallback(std::function<void(Connection*)> const& callback);
    Channel* GetChannel() const {
        return channel_;
    }
    void Close();

 private:
    EventLoop* loop_;
    Socket* sock_;
    Channel* channel_{nullptr};
    std::function<void(Socket*)> delete_connection_callback_;
    std::function<void(Connection*)> read_connect_callback_;
    std::function<void(Connection*)> write_connect_callback_;

};

#endif //MYSERVER_CONNECTION_H