/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#include "Server.h"
#include "Acceptor.h"
#include "Connection.h"
#include "EventLoop.h"
#include "Socket.h"
#include "ThreadPool.h"

Server::Server() : main_reactor_(nullptr), acceptor_(nullptr), thread_pool_(nullptr) {
    main_reactor_ = new EventLoop(0);
    acceptor_ = new Acceptor(main_reactor_); // acceptor绑定主reactor
    // 建立连接模块抽象分离出来，利用回调函数得出机制实现类似虚函数的多态，建立连接的具体逻辑是服务器设定的，所以具体逻辑作为回调函数在服务器类中定义
    // std::function<void(Socket *)> cb = std::bind(&Server::NewConnection, this, std::placeholders::_1);
    // 建立连接后所进行的工作由Server来定制，Accept值负责帮助建立连接
    std::function<void(Socket *)> cb = [this](auto && PH1) { NewConnection(std::forward<decltype(PH1)>(PH1)); };
    acceptor_->SetNewConnectionCallback(cb);

    // 创建线程池
    int size = static_cast<int>(std::thread::hardware_concurrency());
    thread_pool_ = new ThreadPool(size);
    for (int i = 0; i < size; ++i) { // 每一个线程对应一个从reactor
        EventLoop* loop = new EventLoop(i+1);
        sub_reactors_.push_back(loop);
        std::function<void()> sub_loop = [capture0 = sub_reactors_[i]] { capture0->Loop(); };
        thread_pool_->Add(std::move(sub_loop));
    }
}
// 析构服务器是一件很麻烦的事情，必须有逻辑的进行
Server::~Server() {
    // 等待停止 主reactor，停止分发任务
    main_reactor_->StopLoop();
    delete main_reactor_; main_reactor_ = nullptr;
    // 等待停止各个 从reactor，停止接收任务
    for (EventLoop* sub_reactor : sub_reactors_) {
        sub_reactor->StopLoop();
        delete sub_reactor; sub_reactor = nullptr;
    }
    delete acceptor_; acceptor_ = nullptr;
    delete thread_pool_; thread_pool_ = nullptr;
    printf("close connection num: %d ...\n", int(connections_.size()));
    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
        delete it->second;
    }
    connections_.clear();

//    if (monitor_.joinable()) {
//        monitor_.join();
//    }
}

// 这个函数需要由主reactor的连接建立部分调用，用来分配任务到线程池，、
// 由此可以看出，主reactor是在主线程中的，线程池专心用来处理任务
// 此处需要设计相应的线程池调度算法，线程池有Server管理
// 一个新的Connection类在此时被构造出来，用来承接一个分发出来的任务，承接任务的方式还是通过回调函数
// 这个回调函数实在服务启动的主流中设计的，这个非常符合实际，业务多种多样是客户端决定的，也就是在Server类外决定的
void Server::NewConnection(Socket *sock) {
    //  ErrorIf(sock->GetFd() == -1, "new connection error");
    if (-1 == sock->GetFd()) {
        std::cerr << "new connection error" << std::endl;
        return;
    }
    uint64_t random = sock->GetFd() % sub_reactors_.size();
    Connection *conn;
    // 分发子reactor，建立连接需要绑定连接本身的socket fd与分发到的reactor
    conn = new Connection(sub_reactors_[random], sock);
    std::function<void(Socket* )> cb = [this](auto && PH1) { DeleteConnection(std::forward<decltype(PH1)>(PH1)); };
    conn->SetDeleteConnectionCallback(cb); // 设置连接取消的回调函数
    conn->SetReadConnectCallback(read_connect_callback_); // 设置主要业务的回调函数
    conn->SetWriteConnectCallback(write_connect_callback_);
    connections_[sock->GetFd()] = conn;
    conn->EnableRead();
}

void Server::DeleteConnection(Socket *sock) {
    int sockfd = sock->GetFd();
//    std::unique_lock<std::mutex> lock(mutex_);
    auto it = connections_.find(sockfd);
        if (it != connections_.end()) {
        Connection *conn = connections_[sockfd];
        connections_.erase(sockfd);
        delete conn;
    }
}

void Server::ReadConnect(std::function<void(Connection*)> fn) { read_connect_callback_ = std::move(fn); }
void Server::WriteConnect(std::function<void(Connection*)> fn) { write_connect_callback_ = std::move(fn); }

void Server::Run() {
    main_reactor_->Loop();
}