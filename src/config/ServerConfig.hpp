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
    void setClientMaxBody(const String& c);
    bool setServerName(const VectorString& name);
    bool setRoot(const VectorString& root);
    void setRoot(const String& root);
    bool setListen(const VectorString& l);
    bool setErrorPage(const VectorString& values);
    void addLocation(const LocationConfig& loc);

    // getters
    int                         getPort(size_t index = 0) const;
    String                      getInterface(size_t index = 0) const;
    const VectorListenAddress&  getListenAddresses() const;
    bool                        hasPort(int port) const;
    VectorLocationConfig&       getLocations(); // to set parameter in locations from server
    const VectorLocationConfig& getLocations() const;
    String                      getServerName(size_t index = 0) const;
    const VectorString&         getServerNames() const;
    bool                        hasServerName(const String& name) const;
    String                      getRoot() const;
    VectorString                getIndexes() const;
    String                      getClientMaxBody() const;
    const MapIntString&         getErrorPages() const;
    String                      getErrorPage(int code) const;
    bool                        hasErrorPage(int code) const;

   private:
    // required server parameters
    VectorListenAddress  listenAddresses;
    VectorLocationConfig locations; // it least one location

    // optional server parameters
    VectorString serverNames;       // default: empty, can have multiple names
    String       root;              // default: use for location if not set(be required)
    VectorString indexes;           // default: "index.html"
    String       clientMaxBodySize; // default: "1M" or inherited from http config
    MapIntString errorPages;        // maps error code to page path
};
#endif