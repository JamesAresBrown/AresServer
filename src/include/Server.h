/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_SERVER_H
#define MYSERVER_SERVER_H

#include <functional>
#include <map>
#include <vector>
#include "ThreadPool.h"
class EventLoop;
class Socket;
class Acceptor;
class Connection;
class ThreadPool;
class Server {
private:
    // 这些私有成员变量，实际上是服务器的主要模块，为了更好的扩展性，这些模块需要尽可能解耦
    EventLoop* main_reactor_; // 主reactor 服务器一般只有一个main Reactor，有很多个sub Reactor。 main Reactor只负责Acceptor建立新连接，然后将这个连接分配给一个sub Reactor。
    Acceptor* acceptor_{}; // 连接建立器 Acceptor由且只由mainReactor负责
    std::map<int, Connection*> connections_; // 连接序列，红黑树字典
    std::vector<EventLoop*> sub_reactors_; // 从reactor 服务器管理一个线程池，每一个sub Reactor由一个线程来负责Connection上的事件循环，事件执行也在这个线程中完成。
    ThreadPool* thread_pool_; // 线程池 （有服务器管理，）
    std::function<void(Connection*)> read_connect_callback_ = [](auto && ){};
    std::function<void(Connection*)> write_connect_callback_ = [](auto && ){};
//    std::mutex mutex_;
//    std::thread monitor_;
    const unsigned short CONNECTION_TIME_OUT = 60;

public:
    explicit Server();
    ~Server();

    Server(const Server&) = delete; /* NOLINT */
    Server& operator=(const Server&) = delete; /* NOLINT */
    Server(Server&&) = delete; /* NOLINT */
    Server& operator=(Server&&) = delete; /* NOLINT */


    // 我认为 Connection 应该由用户自定义，一个服务器不应该仅仅持有一种Connection
    // 所以 on_connect_callback_ 这个类成员设计的不合理，Server应该维护一个connections_就够了
    // 这里的sock是通过把该函数注册进Accept中获得的，这个逻辑所有连接都是一样的，
    // 先建立连接后，才能进行任务的分发，所以可以把Connect设计成，在Server外定义具体业务，
    void NewConnection(Socket *sock);
    void DeleteConnection(Socket *sock);
    void ReadConnect(std::function<void(Connection *)> fn);
    void WriteConnect(std::function<void(Connection *)> fn);
    int GetConnectionCount() const {
        return connections_.size();
    }
    int GetWorkingThreadCount() const {
        return thread_pool_->GetWorkingThreadCount();
    }
    void Run();
};

#endif //MYSERVER_SERVER_H
