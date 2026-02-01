#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include "../utils/Logger.hpp"
#include "LocationConfig.hpp"

class ListenAddress
{
public:
    ListenAddress() : _interface(""), _port(-1), _serverFd(-1) {}
    ListenAddress(const std::string &iface, int p) : _interface(iface), _port(p), _serverFd(-1) {}

    // Getters
    const std::string &getInterface() const { return _interface; }
    int getPort() const { return _port; }
    int getServerFd() const { return _serverFd; }

    // Setters
    void setServerFd(int fd) { _serverFd = fd; }

private:
    std::string _interface;
    int _port;
    int _serverFd;
};

class ServerConfig
{
public:
    ServerConfig();
    ServerConfig(const ServerConfig &other);
    ServerConfig &operator=(const ServerConfig &other);
    ~ServerConfig();

    // setters
    bool setIndexes(const VectorString &i);
    bool setClientMaxBody(const VectorString &c);
    void setClientMaxBody(const std::string &c);
    bool setServerName(const VectorString &name);
    bool setRoot(const VectorString &root);
    void setRoot(const std::string &root);
    bool setListen(const std::vector<std::string> &l);
    bool setErrorPage(const std::vector<std::string> &values);
    void addLocation(const LocationConfig &loc);

    // getters
    int getPort(size_t index = 0) const;
    std::string getInterface(size_t index = 0) const;
    const std::vector<ListenAddress> &getListenAddresses() const;
    bool hasPort(int port) const;
    std::vector<LocationConfig> &getLocations(); // to set parameter in locations from server
    const std::vector<LocationConfig> &getLocations() const;
    std::string getServerName(size_t index = 0) const;
    const std::vector<std::string> &getServerNames() const;
    bool hasServerName(const std::string &name) const;
    std::string getRoot() const;
    std::vector<std::string> getIndexes() const;
    std::string getClientMaxBody() const;
    const std::map<int, std::string> &getErrorPages() const;
    std::string getErrorPage(int code) const;
    bool hasErrorPage(int code) const;

private:
    // required server parameters
    std::vector<ListenAddress> listenAddresses;
    std::vector<LocationConfig> locations; // it least one location

    // optional server parameters
    std::vector<std::string> serverNames;  // default: empty, can have multiple names
    std::string root;                      // default: use for location if not set(be required)
    std::vector<std::string> indexes;      // default: "index.html"
    std::string clientMaxBodySize;         // default: "1M" or inherited from http config
    std::map<int, std::string> errorPages; // maps error code to page path
};
#endif