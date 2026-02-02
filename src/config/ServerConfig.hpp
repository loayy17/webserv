#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"
#include "ListenAddressConfig.hpp"
#include "LocationConfig.hpp"

class ServerConfig {
   public:
    ServerConfig();
    ServerConfig(const ServerConfig& other);
    ServerConfig& operator=(const ServerConfig& other);
    ~ServerConfig();

    // setters
    bool setIndexes(const VectorString& i);
    bool setClientMaxBody(const VectorString& c);
    void setClientMaxBody(const std::string& c);
    bool setServerName(const VectorString& name);
    bool setRoot(const VectorString& root);
    void setRoot(const std::string& root);
    bool setListen(const VectorString& l);
    bool setErrorPage(const VectorString& values);
    void addLocation(const LocationConfig& loc);

    // getters
    int                         getPort(size_t index = 0) const;
    std::string                 getInterface(size_t index = 0) const;
    const VectorListenAddress&  getListenAddresses() const;
    bool                        hasPort(int port) const;
    VectorLocationConfig&       getLocations(); // to set parameter in locations from server
    const VectorLocationConfig& getLocations() const;
    std::string                 getServerName(size_t index = 0) const;
    const VectorString&         getServerNames() const;
    bool                        hasServerName(const std::string& name) const;
    std::string                 getRoot() const;
    VectorString                getIndexes() const;
    std::string                 getClientMaxBody() const;
    const MapIntString&         getErrorPages() const;
    std::string                 getErrorPage(int code) const;
    bool                        hasErrorPage(int code) const;

   private:
    // required server parameters
    VectorListenAddress  listenAddresses;
    VectorLocationConfig locations; // it least one location

    // optional server parameters
    VectorString serverNames;       // default: empty, can have multiple names
    std::string  root;              // default: use for location if not set(be required)
    VectorString indexes;           // default: "index.html"
    std::string  clientMaxBodySize; // default: "1M" or inherited from http config
    MapIntString errorPages;        // maps error code to page path
};
#endif