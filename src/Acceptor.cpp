/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */

#include "Acceptor.h"
#include <cstring>

#include "Channel.h"
#include "Socket.h"
#include "util.h"

Acceptor::Acceptor(EventLoop *loop) : loop_(loop), sock_(nullptr), channel_(nullptr) {
    sock_ = new Socket();
//    InetAddress *addr;
//    addr = new InetAddress("10.0.4.14", 1234);
    // 准备服务器地址结构体
    struct sockaddr_in server_addr{};
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 监听所有网络接口
    server_addr.sin_port = htons(80); // 使用端口 80 http

    sock_->SetReuseadder(); // 关闭后立即重用该地址。这样可以避免处于TIME_WAIT状态的套接字占用端口，从而加快端口的回收速度。
//    ErrorIf(sock_->Bind(addr) == -1, "socket bind error");
    ErrorIf(
            bind(sock_->GetFd(), (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1,
            "socket bind error"
            );
    ErrorIf(
            sock_->Listen() == -1,
            "socket listen error"
            );
    channel_ = new Channel(loop_, sock_->GetFd()); // 这里绑定的loop是主reactor
    std::function<void()> cb = [this] { AcceptConnection(); };
    channel_->SetReadCallback(cb);
    // 主reactor负责接收连接消息，采用LT模式
    channel_->EnableRead();
//    delete addr;
}

Acceptor::~Acceptor() {
    delete channel_;
    delete sock_;
}

void Acceptor::AcceptConnection() {
    InetAddress* client_addr;
    client_addr = new InetAddress();
    Socket* client_sock;
    int ret = sock_->Accept(client_addr);
    if (-1 == ret) {
        return;
    }
    client_sock = new Socket(ret);
    printf("New client fd %d! IP:%s  Port:%d\n", client_sock->GetFd(), client_addr->GetIp(), client_addr->GetPort());
    client_sock->SetNonBlocking(); // 新接受到的连接的socket设置为非阻塞式
    new_connection_callback_(client_sock);
    delete client_addr;
}
// 设置处理连接的函数，该函数值接收一个参数是服务端连接的Socket
void Acceptor::SetNewConnectionCallback(std::function<void(Socket *)> const &callback) {
  new_connection_callback_ = callback;
}
