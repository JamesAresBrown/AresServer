/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_SOCKET_H
#define MYSERVER_SOCKET_H
#include <arpa/inet.h>

class InetAddress {
public:
    InetAddress();
    InetAddress(const char *ip, uint16_t port);
    ~InetAddress() = default;

    InetAddress(const InetAddress&) = delete; /* NOLINT */
    InetAddress& operator=(const InetAddress&) = delete; /* NOLINT */
    InetAddress(InetAddress&&) = delete; /* NOLINT */
    InetAddress& operator=(InetAddress&&) = delete; /* NOLINT */

    void SetAddr(sockaddr_in addr);
    sockaddr_in GetAddr();
    const char *GetIp() const;
    uint16_t GetPort() const;

private:
    struct sockaddr_in addr_ {};
};

class Socket {
private:
  int fd_{-1};

public:
    Socket();
    explicit Socket(int fd);
    ~Socket();

    Socket(const Socket&) = delete; /* NOLINT */
    Socket& operator=(const Socket&) = delete; /* NOLINT */
    Socket(Socket&&) = delete; /* NOLINT */
    Socket& operator=(Socket&&) = delete; /* NOLINT */

    int Bind(InetAddress *addr) const;
    int Listen() const;
    int Accept(InetAddress *addr) const;

    int Connect(InetAddress *addr) const;
    int Connect(const char *ip, uint16_t port) const;

    void SetNonBlocking() const;
    void SetReuseadder() const;
    bool IsNonBlocking() const;
    int GetFd() const;
};

#endif //MYSERVER_SOCKET_H
