#include "EpollManager.hpp"
#include "../utils/Logger.hpp"
#include <iostream>

EpollManager::EpollManager() : _epollFd(-1) {}

EpollManager::~EpollManager() {
    if (_epollFd != -1) {
        close(_epollFd);
    }
}

bool EpollManager::init() {
    _epollFd = epoll_create(1);
    if (_epollFd == -1) {
        return Logger::error("Failed to create epoll instance");
    }
    _events.resize(MAX_CONNECTIONS);
    return true;
}

void EpollManager::addFd(int fd, uint32_t events) {
    if (fd < 0) return;

    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;

    std::map<int, uint32_t>::iterator it = _fdEvents.find(fd);
    if (it != _fdEvents.end()) {
        if (it->second == events) return;
        if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) == -1) {
            Logger::error("Failed to modify fd in epoll");
        }
    } else {
        if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev) == -1) {
            Logger::error("Failed to add fd to epoll");
        }
    }
    _fdEvents[fd] = events;
}

void EpollManager::modifyFd(int fd, uint32_t events) {
    addFd(fd, events);
}

void EpollManager::removeFd(int fd) {
    if (fd < 0) return;

    if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        // Might already be closed or removed
    }
    _fdEvents.erase(fd);
}

int EpollManager::wait(int timeout) {
    int nfds = epoll_wait(_epollFd, &_events[0], _events.size(), timeout);
    return nfds;
}

int EpollManager::getFd(size_t index) const {
    if (index >= _events.size()) return -1;
    return _events[index].data.fd;
}

uint32_t EpollManager::getEvents(size_t index) const {
    if (index >= _events.size()) return 0;
    return _events[index].events;
}

size_t EpollManager::size() const {
    return _fdEvents.size();
}
