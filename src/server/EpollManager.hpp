#ifndef EPOLLMANAGER_HPP
#define EPOLLMANAGER_HPP

#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <map>
#include "../utils/Types.hpp"
#include "../utils/Constants.hpp"

class EpollManager {
   private:
    int _epollFd;
    std::vector<struct epoll_event> _events;
    std::map<int, uint32_t> _fdEvents;

   public:
    EpollManager();
    ~EpollManager();

    bool init();
    void addFd(int fd, uint32_t events);
    void modifyFd(int fd, uint32_t events);
    void removeFd(int fd);
    int  wait(int timeout);

    int  getFd(size_t index) const;
    uint32_t getEvents(size_t index) const;
    size_t size() const;

   private:
    EpollManager(const EpollManager&);
    EpollManager& operator=(const EpollManager&);
};

#endif
