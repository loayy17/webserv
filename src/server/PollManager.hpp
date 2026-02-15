#ifndef POLLMANAGER_HPP
#define POLLMANAGER_HPP

#include <poll.h>
#include <cstddef>
#include <vector>
#include "../utils/Types.hpp"

class PollManager {
   private:
    std::vector<struct pollfd> fds;

   public:
    PollManager(const PollManager&);
    PollManager& operator=(const PollManager&);
    PollManager();
    ~PollManager();
    void   addFd(int fd, int events);
    void   removeFd(size_t index);
    int    pollConnections(int timeout);
    bool   hasEvent(size_t index, int event) const;
    int    getFd(size_t index) const;
    VectorInt getFds() const;
    size_t size() const;
};

#endif