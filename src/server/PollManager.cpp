#include "PollManager.hpp"

PollManager::PollManager(const PollManager& other) : fds(other.fds), _fdIndex(other._fdIndex) {}

PollManager& PollManager::operator=(const PollManager& other) {
    if (this != &other) {
        fds      = other.fds;
        _fdIndex = other._fdIndex;
    }
    return *this;
}

PollManager::PollManager() {}

PollManager::~PollManager() {
    fds.clear();
    _fdIndex.clear();
}

void PollManager::reserve(size_t n) {
    fds.reserve(n);
}

void PollManager::addFd(int fd, int events) {
    if (fd < 0)
        return;

    std::map<int, size_t>::iterator it = _fdIndex.find(fd);
    if (it != _fdIndex.end()) {
        size_t idx       = it->second;
        fds[idx].events  = events;
        fds[idx].revents = 0;
        return;
    }

    struct pollfd pfd;
    pfd.fd      = fd;
    pfd.events  = events;
    pfd.revents = 0;
    _fdIndex[fd] = fds.size();
    fds.push_back(pfd);
}

void PollManager::removeFd(size_t index) {
    if (index >= fds.size())
        return;

    int removedFd = fds[index].fd;
    _fdIndex.erase(removedFd);

    // Swap with last element and pop (O(1) instead of O(n) erase)
    if (index < fds.size() - 1) {
        fds[index] = fds.back();
        _fdIndex[fds[index].fd] = index;
    }
    fds.pop_back();
}

void PollManager::removeFdByValue(int fd) {
    std::map<int, size_t>::iterator it = _fdIndex.find(fd);
    if (it != _fdIndex.end())
        removeFd(it->second);
}

int PollManager::pollConnections(int timeout) {
    if (fds.empty())
        return 0;

    for (size_t i = 0; i < fds.size(); i++) {
        fds[i].revents = 0;
    }

    return poll(&fds[0], fds.size(), timeout);
}

bool PollManager::hasEvent(size_t index, int event) const {
    if (index >= fds.size())
        return false;
    return (fds[index].revents & event) != 0;
}

int PollManager::getFd(size_t index) const {
    if (index >= fds.size())
        return -1;
    return fds[index].fd;
}

VectorInt PollManager::getFds() const {
    VectorInt result;
    for (size_t i = 0; i < fds.size(); i++) {
        result.push_back(fds[i].fd);
    }
    return result;
}

size_t PollManager::size() const {
    return fds.size();
}
