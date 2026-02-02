#include "ListenAddressConfig.hpp"

ListenAddress::ListenAddress() : _interface(""), _port(-1), _serverFd(-1) {}
ListenAddress::ListenAddress(const ListenAddress& other) : _interface(other._interface), _port(other._port), _serverFd(other._serverFd) {}
ListenAddress& ListenAddress::operator=(const ListenAddress& other) {
    if (this != &other) {
        _interface = other._interface;
        _port      = other._port;
        _serverFd  = other._serverFd;
    }
    return *this;
}
ListenAddress::~ListenAddress() {}
ListenAddress::ListenAddress(const std::string& iface, int p) : _interface(iface), _port(p), _serverFd(-1) {}
const std::string& ListenAddress::getInterface() const {
    return _interface;
}
int ListenAddress::getPort() const {
    return _port;
}
int ListenAddress::getServerFd() const {
    return _serverFd;
}
void ListenAddress::setServerFd(int fd) {
    _serverFd = fd;
}
