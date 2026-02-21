#include "ServerConfig.hpp"

ServerConfig::ServerConfig() : listenAddresses(), locations(), serverNames(), root(""), indexes(), clientMaxBodySize(-1), errorPages() {}

ServerConfig::ServerConfig(const ServerConfig& other)
    : listenAddresses(other.listenAddresses),
      locations(other.locations),
      serverNames(other.serverNames),
      root(other.root),
      indexes(other.indexes),
      clientMaxBodySize(other.clientMaxBodySize),
      errorPages(other.errorPages) {}

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {
        listenAddresses   = other.listenAddresses;
        locations         = other.locations;
        serverNames       = other.serverNames;
        root              = other.root;
        indexes           = other.indexes;
        clientMaxBodySize = other.clientMaxBodySize;
        errorPages        = other.errorPages;
    }
    return *this;
}

ServerConfig::~ServerConfig() {}

bool ServerConfig::setIndexes(const VectorString& i) {
    if (!indexes.empty())
        return Logger::error("duplicate index directive");
    if (i.empty())
        return Logger::error("index requires at least one value");
    indexes = i;
    return true;
}

bool ServerConfig::setClientMaxBody(const VectorString& c) {
    if (clientMaxBodySize != -1)
        return Logger::error("Duplicate client_max_body_size");
    if (!requireSingleValue(c, "client_max_body_size"))
        return false;
    //split M K G m k g from c[0]
    size_t dummy;
    char   unit       = c[0][c[0].size() - 1];
    String numberPart = std::isdigit(unit) ? c[0] : c[0].substr(0, c[0].size() - 1);
    if (!stringToType<size_t>(numberPart, dummy))
        return Logger::error("invalid client_max_body_size: " + c[0]);
    clientMaxBodySize = convertMaxBodySize(c[0]);
    return true;
}

void ServerConfig::setClientMaxBody(ssize_t c) {
    clientMaxBodySize = c;
}

bool ServerConfig::setServerName(const VectorString& names) {
    if (!serverNames.empty())
        return Logger::error("duplicate server_name directive");
    if (names.empty())
        return Logger::error("server_name requires at least one value");
    serverNames = names;
    return true;
}

bool ServerConfig::setRoot(const VectorString& r) {
    if (!root.empty())
        return Logger::error("duplicate root");
    if (!requireSingleValue(r, "root"))
        return false;

    String newRoot = r[0];
    if (newRoot.size() > 1 && newRoot[newRoot.size() - 1] == SLASH)
        newRoot.erase(newRoot.size() - 1);
    root = newRoot;
    return true;
}

void ServerConfig::setRoot(const String& r) {
    root = r;
}

bool ServerConfig::listenExists(const ListenAddress& addr) const {
    for (size_t i = 0; i < listenAddresses.size(); ++i)
        if (listenAddresses[i].getInterface() == addr.getInterface() && listenAddresses[i].getPort() == addr.getPort())
            return true;
    return false;
}

bool ServerConfig::setListen(const VectorString& l) {
    if (!requireSingleValue(l, "listen"))
        return false;
    String interface, portStr;
    if (!splitByChar(l[0], interface, portStr, COLON))
        return Logger::error("invalid listen format");
    int port;
    if (!stringToType<int>(portStr, port) || port < 1 || port > 65535)
        return Logger::error("invalid port: " + portStr);
    if (interface == "localhost")
        interface = "127.0.0.1";
    ListenAddress addr(interface, port);
    if (listenExists(addr))
        return Logger::error("duplicate listen address: " + l[0]);
    listenAddresses.push_back(addr);
    return true;
}

bool ServerConfig::setErrorPage(const VectorString& values) {
    if (values.size() < 2)
        return Logger::error("error_page requires at least one code and a path");
    const String& pagePath = values.back();
    for (size_t i = 0; i < values.size() - 1; ++i) {
        int code;
        if (!stringToType<int>(values[i], code) || code < 100 || code > 599)
            return Logger::error("invalid HTTP status code: " + values[i]);
        errorPages[code] = pagePath;
    }
    return true;
}

void ServerConfig::addLocation(const LocationConfig& loc) {
    locations.push_back(loc);
    LocationConfig& newLoc = locations.back();
    if (newLoc.getRoot().empty())
        newLoc.setRoot(root);
    if (newLoc.getIndexes().empty())
        newLoc.setIndexes(indexes);
}

int ServerConfig::getPort(size_t index) const {
    return (index < listenAddresses.size()) ? listenAddresses[index].getPort() : -1;
}

String ServerConfig::getInterface(size_t index) const {
    return (index < listenAddresses.size()) ? listenAddresses[index].getInterface() : "";
}

const VectorListenAddress& ServerConfig::getListenAddresses() const {
    return listenAddresses;
}

bool ServerConfig::hasPort(int port) const {
    for (size_t i = 0; i < listenAddresses.size(); ++i) {
        if (listenAddresses[i].getPort() == port)
            return true;
    }
    return false;
}

VectorLocationConfig& ServerConfig::getLocations() {
    return locations;
}

const VectorLocationConfig& ServerConfig::getLocations() const {
    return locations;
}

VectorString ServerConfig::getIndexes() const {
    return indexes;
}

String ServerConfig::getServerName(size_t index) const {
    return (index < serverNames.size()) ? serverNames[index] : "";
}

const VectorString& ServerConfig::getServerNames() const {
    return serverNames;
}

bool ServerConfig::hasServerName(const String& name) const {
    return isKeyInVector(name, serverNames);
}

String ServerConfig::getRoot() const {
    return root;
}

ssize_t ServerConfig::getClientMaxBody() const {
    return clientMaxBodySize;
}

const MapIntString& ServerConfig::getErrorPages() const {
    return errorPages;
}

String ServerConfig::getErrorPage(int code) const {
    MapIntString::const_iterator it = errorPages.find(code);
    return (it != errorPages.end()) ? it->second : "";
}

bool ServerConfig::hasErrorPage(int code) const {
    return errorPages.find(code) != errorPages.end();
}