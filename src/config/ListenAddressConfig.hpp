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
//
/*
listen 192.0.0.1:8080
listen 193.0.0.1:8080
listen 194.0.0.1:8080

*/