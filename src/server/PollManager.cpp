#include "PollManager.hpp"

PollManager::PollManager(const PollManager& other) : fds(other.fds) {}

PollManager& PollManager::operator=(const PollManager& other) {
    if (this != &other) {
        fds = other.fds;
    }
    return *this;
}

PollManager::PollManager() {}

PollManager::~PollManager() {
    fds.clear();
}

void PollManager::addFd(int fd, int events) {
    if (fd < 0) return;
    
    for (size_t i = 0; i < fds.size(); i++) {
        if (fds[i].fd == fd) {
            fds[i].events = events;
            fds[i].revents = 0;
            return;
        }
    }
    
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = events;
    pfd.revents = 0;
    fds.push_back(pfd);
}

void PollManager::removeFd(size_t index) {
    if (index >= fds.size()) return;
    
    if (index != fds.size() - 1) {
        fds[index] = fds[fds.size() - 1];
    }
    fds.pop_back();
}

int PollManager::pollConnections(int timeout) {
    if (fds.empty()) return 0;
    
    for (size_t i = 0; i < fds.size(); i++) {
        fds[i].revents = 0;
    }
    
    return poll(&fds[0], fds.size(), timeout);
}

bool PollManager::hasEvent(size_t index, int event) const {
    if (index >= fds.size()) return false;
    return (fds[index].revents & event) != 0;
}

int PollManager::getFd(size_t index) const {
    if (index >= fds.size()) return -1;
    return fds[index].fd;
}

size_t PollManager::size() const {
    return fds.size();
}
