#ifndef EPOLL_MANAGER_HPP
#define EPOLL_MANAGER_HPP

#ifdef __linux__
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include "../utils/Logger.hpp"

#define MAX_EVENTS 1024

class EpollManager {
private:
    int _epollFd;
    struct epoll_event _events[MAX_EVENTS];
    std::vector<int> _fds;

public:
    EpollManager() : _epollFd(-1) {
        _epollFd = epoll_create1(0);
        if (_epollFd == -1) {
            Logger::error("Failed to create epoll instance");
        }
    }

    ~EpollManager() {
        if (_epollFd != -1) {
            close(_epollFd);
        }
    }

    bool addFd(int fd, uint32_t events) {
        struct epoll_event ev;
        ev.events = events;
        ev.data.fd = fd;
        if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev) == -1) {
            if (errno == EEXIST) {
                return modifyFd(fd, events);
            }
            return false;
        }
        _fds.push_back(fd);
        return true;
    }

    bool modifyFd(int fd, uint32_t events) {
        struct epoll_event ev;
        ev.events = events;
        ev.data.fd = fd;
        return epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) != -1;
    }

    bool removeFd(int fd) {
        if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1) {
            return false;
        }
        for (size_t i = 0; i < _fds.size(); ++i) {
            if (_fds[i] == fd) {
                _fds.erase(_fds.begin() + i);
                break;
            }
        }
        return true;
    }

    int wait(int timeout) {
        return epoll_wait(_epollFd, _events, MAX_EVENTS, timeout);
    }

    int getEventFd(int index) const {
        return _events[index].data.fd;
    }

    uint32_t getEvents(int index) const {
        return _events[index].events;
    }

    int getEpollFd() const { return _epollFd; }
};

#endif // __linux__
#endif
