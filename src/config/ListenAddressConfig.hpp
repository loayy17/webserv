#ifndef LISTEN_ADDRESS_CONFIG_HPP
#define LISTEN_ADDRESS_CONFIG_HPP
#include <iostream>
class ListenAddress {
   public:
    ListenAddress();
    ListenAddress(const ListenAddress& other);
    ListenAddress& operator=(const ListenAddress& other);
    ListenAddress(const std::string& iface, int p);
    ~ListenAddress();
    // Getters
    const std::string& getInterface() const;
    int                getPort() const;
    int                getServerFd() const;

    // Setters
    void setServerFd(int fd);

   private:
    std::string _interface;
    int         _port;
    int         _serverFd;
};

#endif
//
/*
listen 192.0.0.1:8080
listen 193.0.0.1:8080
listen 194.0.0.1:8080

*/