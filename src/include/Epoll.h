/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_EPOLL_H
#define MYSERVER_EPOLL_H
#include <vector>
#include <sys/epoll.h>

class Channel;
class Epoll {
public:
    Epoll();
    ~Epoll();

    Epoll(const Epoll&) = delete; /* NOLINT */
    Epoll& operator=(const Epoll&) = delete; /* NOLINT */
    Epoll(Epoll&&) = delete; /* NOLINT */
    Epoll& operator=(Epoll&&) = delete; /* NOLINT */


    void UpdateChannel(Channel *ch);
    void DeleteChannel(Channel *ch);

    std::vector<Channel *> Poll(int timeout = -1);

public:
    int epfd_{1};
    struct epoll_event* events_{nullptr};
};

#endif //MYSERVER_EPOLL_H
