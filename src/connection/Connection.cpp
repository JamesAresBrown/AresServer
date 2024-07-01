/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#include "Connection.h"
#include "Channel.h"
#include "Socket.h"

Connection::Connection(EventLoop *loop, Socket *sock) : loop_(loop), sock_(sock) {
  if (loop_ != nullptr) {
    channel_ = new Channel(loop_, sock->GetFd());
//    channel_->EnableRead();
    // Connection在从reactor下工作，采用ET模式，对于读区域的操作要保证处理完整
    channel_->UseET();
  }
//  state_ = State::Connected;
}

void Connection::EnableRead() {
    if (loop_ != nullptr) {
//        channel_ = new Channel(loop_, sock_->GetFd());
        channel_->EnableRead();
//        channel_->UseET();
    }
}

Connection::~Connection() {
    if (application_ != nullptr) {
        delete application_;
        application_ = nullptr;
    }
    printf("close conn fd: %d\n", sock_->GetFd());
    delete sock_;
}

void Connection::Close() {
    delete_connection_callback_(sock_);
}

void Connection::SetDeleteConnectionCallback(std::function<void(Socket *)> const &callback) {
  delete_connection_callback_ = callback;
}
void Connection::SetReadConnectCallback(std::function<void(Connection *)> const &callback) {
  read_connect_callback_ = callback;
  channel_->SetReadCallback([this]() { read_connect_callback_(this); });
}
void Connection::SetWriteConnectCallback(std::function<void(Connection *)> const &callback) {
  write_connect_callback_ = callback;
  channel_->SetWriteCallback([this]() { write_connect_callback_(this); });
}
