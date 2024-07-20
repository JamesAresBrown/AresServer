/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#include "Socket.h"
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>

#include "util.h"

Socket::Socket() {
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  ErrorIf(fd_ == -1, "socket create error");
//    // 设置 TCP Keep-Alive
//    int keepAlive = 1; // 开启 Keep-Alive
//    int keepIdle = 60; // 开始首次 Keep-Alive 探测前的 TCP 空闲时间
//    int keepInterval = 5; // 两次 Keep-Alive 探测间的时间间隔
//    int keepCount = 3; // 重新传输 Keep-Alive 探测的次数
//    setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive));
//    setsockopt(fd_, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle));
//    setsockopt(fd_, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval));
//    setsockopt(fd_, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(keepCount));
}
Socket::Socket(int fd) : fd_(fd) {
    ErrorIf(fd_ == -1, "socket create error");
}

Socket::~Socket() {
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
}

int Socket::Bind(InetAddress *addr) const {
  struct sockaddr_in tmp_addr = addr->GetAddr();
//  ErrorIf(bind(fd_, (sockaddr *)&tmp_addr, sizeof(tmp_addr)) == -1, "socket bind error");
    return bind(fd_, (sockaddr *)&tmp_addr, sizeof(tmp_addr));
}

int Socket::Listen() const {
//    ErrorIf(::listen(fd_, SOMAXCONN) == -1, "socket listen error");
    return listen(fd_, SOMAXCONN);
}
void Socket::SetNonBlocking() const {
    fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | O_NONBLOCK);
}
void Socket::SetReuseadder() const {
    int iSockOptVal = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &iSockOptVal, sizeof(iSockOptVal));
}
bool Socket::IsNonBlocking() const {
    return (fcntl(fd_, F_GETFL) & O_NONBLOCK) != 0;
}
int Socket::Accept(InetAddress *addr) const {
  // for server socket
  int clnt_sockfd;
  struct sockaddr_in tmp_addr {};
  socklen_t addr_len = sizeof(tmp_addr);
  if (IsNonBlocking()) { // 判断是否是非阻塞的
    // 非阻塞接收
    while (true) {
      clnt_sockfd = accept(fd_, (sockaddr *)&tmp_addr, &addr_len);
      if (clnt_sockfd == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
        // printf("no connection yet\n");
        continue;
      }
      if (clnt_sockfd == -1) {
//        ErrorIf(true, "socket accept error");
          return -1;
      } else {
        break;
      }
    }
  } else {
      // 阻塞接收
    clnt_sockfd = accept(fd_, (sockaddr *)&tmp_addr, &addr_len);
//    ErrorIf(clnt_sockfd == -1, "socket accept error");
    if (-1 == clnt_sockfd) {
        return -1;
    }
  }
  addr->SetAddr(tmp_addr);
  return clnt_sockfd;
}

int Socket::Connect(InetAddress *addr) const {
  // for client socket
  struct sockaddr_in tmp_addr = addr->GetAddr();
  if (fcntl(fd_, F_GETFL) & O_NONBLOCK) { // 观察阻塞模式
    // 非阻塞情况下要循环
    while (true) {
      int ret = connect(fd_, (sockaddr *)&tmp_addr, sizeof(tmp_addr));
      if (ret == 0) {
        break;
      }
      if (ret == -1 && (errno == EINPROGRESS)) {
        continue;
        /* 连接非阻塞式sockfd建议的做法：
            The socket is nonblocking and the connection cannot be
          completed immediately.  (UNIX domain sockets failed with
          EAGAIN instead.)  It is possible to select(2) or poll(2)
          for completion by selecting the socket for writing.  After
          select(2) indicates writability, use getsockopt(2) to read
          the SO_ERROR option at level SOL_SOCKET to determine
          whether connect() completed successfully (SO_ERROR is
          zero) or unsuccessfully (SO_ERROR is one of the usual
          error codes listed here, explaining the reason for the
          failure).
          套接字是非阻塞的，连接不能立即完成。
          （UNIX 域套接字失败时会返回 EAGAIN。）可以使用 select(2) 或 poll(2) 来选择套接字以便完成连接。
          在 select(2) 指示可写性之后，使用 getsockopt(2) 来读取 SOL_SOCKET 级别的 SO_ERROR 选项，
          以确定 connect() 是否成功完成（SO_ERROR 为零）或失败（SO_ERROR 是以下列出的常见错误代码之一，解释了失败的原因）。
          这里为了简单、不断连接直到连接完成，相当于阻塞式
        */
      }
      if (ret == -1) {
//        ErrorIf(true, "socket connect error");
          return -1;
      }
    }
  } else {
//    ErrorIf(connect(fd_, (sockaddr *)&tmp_addr, sizeof(tmp_addr)) == -1, "socket connect error");
      return connect(fd_, (sockaddr *)&tmp_addr, sizeof(tmp_addr));
  }
}

int Socket::Connect(const char *ip, uint16_t port) const {
  InetAddress *addr;
  addr = new InetAddress(ip, port);
  int ret = Connect(addr);
  delete addr;
    return ret;
}

int Socket::GetFd() const { return fd_; }

InetAddress::InetAddress() = default;
InetAddress::InetAddress(const char *ip, uint16_t port) {
  memset(&addr_, 0, sizeof(addr_));
  addr_.sin_family = AF_INET;
  addr_.sin_addr.s_addr = inet_addr(ip);
  addr_.sin_port = htons(port);
}

void InetAddress::SetAddr(sockaddr_in addr) { addr_ = addr; }

sockaddr_in InetAddress::GetAddr() { return addr_; }

const char *InetAddress::GetIp() const { return inet_ntoa(addr_.sin_addr); }

uint16_t InetAddress::GetPort() const { return ntohs(addr_.sin_port); }
