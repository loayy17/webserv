#ifndef POLLMANAGER_HPP
#define POLLMANAGER_HPP

#include <poll.h>
#include <cstddef>
#include <map>
#include <vector>
#include "../utils/Types.hpp"

class PollManager {
   private:
    std::vector<struct pollfd> fds;
    std::map<int, size_t>     _fdIndex;

   public:
    PollManager(const PollManager&);
    PollManager& operator=(const PollManager&);
    PollManager();
    ~PollManager();
    void      addFd(int fd, int events);
    void      removeFd(size_t index);
    void      removeFdByValue(int fd);
    int       pollConnections(int timeout);
    bool      hasEvent(size_t index, int event) const;
    int       getFd(size_t index) const;
    VectorInt getFds() const;
    size_t    size() const;
};

#endif