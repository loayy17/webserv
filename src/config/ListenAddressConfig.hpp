#ifndef LISTEN_ADDRESS_CONFIG_HPP
#define LISTEN_ADDRESS_CONFIG_HPP
#include <iostream>
#include "../utils/Types.hpp"
class ListenAddress {
   public:
    ListenAddress();
    ListenAddress(const ListenAddress& other);
    ListenAddress& operator=(const ListenAddress& other);
    ListenAddress(const String& iface, int p);
    ~ListenAddress();
    // Getters
    const String& getInterface() const;
    int           getPort() const;
    int           getServerFd() const;

    // Setters
    void setServerFd(int fd);

   private:
    String _interface;
    int    _port;
    int    _serverFd;
};

#endif