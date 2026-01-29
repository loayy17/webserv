#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP
#include <cstdlib>
#include <iostream>
#include <vector>
#include "../utils/Logger.hpp"
#include "LocationConfig.hpp"

class ServerConfig {
    public:
        ServerConfig();
        ServerConfig(const ServerConfig &other);
        ServerConfig &operator=(const ServerConfig& other);
        ~ServerConfig();

    // setters
    bool setIndexes(const std::vector<std::string>& i);
    bool setClientMaxBody(const std::vector<std::string>& c);
    void setClientMaxBody(const std::string& c);
    bool setServerName(const std::vector<std::string>& name);
    bool setRoot(const std::vector<std::string>& root);
    void setRoot(const std::string& root);
    bool setListen(const std::vector<std::string>& l);
    void addLocation(const LocationConfig& loc);

    //getters
    int                                getPort() const;
    std::string                        getInterface() const;
    std::vector<LocationConfig>&       getLocations(); // to set parameter in locations from server
    const std::vector<LocationConfig>& getLocations() const;
    std::string                        getServerName() const;
    std::string                        getRoot() const;
    std::vector<std::string>           getIndexes() const;
    std::string                        getClientMaxBody() const;

   private:
    // required server parameters
    int                         port;
    std::string                 interface;
    std::vector<LocationConfig> locations; // it least one location

    // optional server parameters
    std::string              serverName;        // default: ""
    std::string              root;              // default: use for location if not set(be required)
    std::vector<std::string> indexes;           // default: "index.html"
    std::string              clientMaxBodySize; // default: "1M" or inherited from http config
};
#endif