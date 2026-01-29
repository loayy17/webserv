#include "ServerConfig.hpp"


ServerConfig::ServerConfig() :
    port(-1),
    interface(""),
    locations(),
    serverName(""),
    root(""),
    indexes(std::vector<std::string>(1, "index.html")),
    clientMaxBodySize("")
{}

ServerConfig::ServerConfig(const ServerConfig& other) :
    port(other.port),
    interface(other.interface),
    locations(other.locations),
    serverName(other.serverName),
    root(other.root),
    indexes(other.indexes),
    clientMaxBodySize(other.clientMaxBodySize)
{}

ServerConfig &ServerConfig::operator=(const ServerConfig &other)
{
    if (this != &other) {
        port              = other.port;
        interface         = other.interface;
        locations         = other.locations;
        serverName        = other.serverName;
        root              = other.root;
        indexes           = other.indexes;
        clientMaxBodySize = other.clientMaxBodySize;
    }
    return *this;
}

ServerConfig::~ServerConfig() {
    indexes.clear();
    locations.clear();
}
// setters
bool ServerConfig::setIndexes(const std::vector<std::string>& i) {
    if (!indexes.empty())
        return Logger::error("duplicate index");
    if (i.empty())
        return Logger::error("index takes at least one value");
    indexes = i;
    return true;
}
bool ServerConfig::setClientMaxBody(const std::vector<std::string>& c) {
    if (!clientMaxBodySize.empty())
        return Logger::error("duplicate client_max_body_size");
    if (c.size() != 1)
        return Logger::error("client_max_body_size takes exactly one value");
    clientMaxBodySize = c[0];
    return true;
}
void ServerConfig::setClientMaxBody(const std::string& c) {
    clientMaxBodySize = c;
}
bool ServerConfig::setServerName(const std::vector<std::string>& name) {
    if (!serverName.empty())
        return Logger::error("duplicate server_name");
    if (name.size() != 1)
        return Logger::error("server_name takes exactly one value");
    serverName = name[0];
    return true;
}
bool ServerConfig::setRoot(const std::vector<std::string>& r) {
    if (!root.empty())
        return Logger::error("duplicate root");
    if (r.size() != 1)
        return Logger::error("root takes exactly one value");
    root = r[0];
    if (root[root.length() - 1] == '/')
        root.erase(root.length() - 1);
    return true;
}
void ServerConfig::setRoot(const std::string& r) {
    root = r;
}
bool ServerConfig::setListen(const std::vector<std::string>& l) {
    if (l.size() != 1)
        return Logger::error("listen takes exactly one value");
    if (!interface.empty() || port != -1)
        return Logger::error("duplicate listen");
    const std::string& v = l[0];
    size_t             c = v.find(':');
    if (c == std::string::npos)
        return Logger::error("invalid listen format");
    std::string portstr = v.substr(c + 1);
    if (portstr.empty())
        return Logger::error("invalid port");
    char* endptr = NULL;
    long  p      = std::strtol(portstr.c_str(), &endptr, 10);
    if (endptr == portstr.c_str() || *endptr != '\0')
        return Logger::error("invalid port");
    if (p < 1 || p > 65535)
        return Logger::error("invalid port");
    interface = v.substr(0, c);
    port      = static_cast<int>(p);
    return true;
}

void ServerConfig::addLocation(const LocationConfig& loc) {
    locations.push_back(loc);
    if (locations.back().getRoot().empty())
        locations.back().setRoot(std::vector<std::string>(1, root));
    if (locations.back().getIndexes().empty())
        locations.back().setIndexes(indexes);
}

//getters
int ServerConfig::getPort() const {
    return port;
}
std::string ServerConfig::getInterface() const {
    return interface;
}
std::vector<LocationConfig>& ServerConfig::getLocations() {
    return locations;
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const {
    return locations;
}
std::vector<std::string> ServerConfig::getIndexes() const {
    return indexes;
}
std::string ServerConfig::getServerName() const {
    return serverName;
}
std::string ServerConfig::getRoot() const {
    return root;
}

std::string ServerConfig::getClientMaxBody() const {
    return clientMaxBodySize;
}