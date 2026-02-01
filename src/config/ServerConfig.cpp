#include "ServerConfig.hpp"

ServerConfig::ServerConfig() : listenAddresses(),
                               locations(),
                               serverNames(),
                               root(""),
                               indexes(),
                               clientMaxBodySize(""),
                               errorPages()
{
}

ServerConfig::ServerConfig(const ServerConfig &other) : listenAddresses(other.listenAddresses),
                                                        locations(other.locations),
                                                        serverNames(other.serverNames),
                                                        root(other.root),
                                                        indexes(other.indexes),
                                                        clientMaxBodySize(other.clientMaxBodySize),
                                                        errorPages(other.errorPages)
{
}

ServerConfig &ServerConfig::operator=(const ServerConfig &other)
{
    if (this != &other)
    {
        listenAddresses = other.listenAddresses;
        locations = other.locations;
        serverNames = other.serverNames;
        root = other.root;
        indexes = other.indexes;
        clientMaxBodySize = other.clientMaxBodySize;
        errorPages = other.errorPages;
    }
    return *this;
}

ServerConfig::~ServerConfig()
{
    indexes.clear();
    locations.clear();
}
// setters
bool ServerConfig::setIndexes(const VectorString &i)
{
    if (!indexes.empty())
        return Logger::error("duplicate index directive");
    if (i.empty())
        return Logger::error("index requires at least one value");
    indexes = i;
    return true;
}
bool ServerConfig::setClientMaxBody(const VectorString &c)
{
    if (!clientMaxBodySize.empty())
        return Logger::error("duplicate client_max_body_size");
    if (c.size() != 1)
        return Logger::error("client_max_body_size takes exactly one value");
    clientMaxBodySize = c[0];
    return true;
}
void ServerConfig::setClientMaxBody(const std::string &c)
{
    clientMaxBodySize = c;
}
bool ServerConfig::setServerName(const std::vector<std::string> &names)
{
    if (!serverNames.empty())
        return Logger::error("duplicate server_name directive");
    if (names.empty())
        return Logger::error("server_name requires at least one value");
    serverNames = names;
    return true;
}
bool ServerConfig::setRoot(const VectorString &r)
{
    if (!root.empty())
        return Logger::error("duplicate root");
    if (r.size() != 1)
        return Logger::error("root takes exactly one value");
    root = r[0];
    if (root[root.length() - 1] == '/')
        root.erase(root.length() - 1);
    return true;
}
void ServerConfig::setRoot(const std::string &r)
{
    root = r;
}
bool ServerConfig::setListen(const VectorString &l)
{
    if (l.size() != 1)
        return Logger::error("listen takes exactly one value");
    const std::string &v = l[0];
    size_t c = v.find(':');
    if (c == std::string::npos)
        return Logger::error("invalid listen format");
    std::string portstr = v.substr(c + 1);
    if (portstr.empty())
        return Logger::error("invalid port");
    char *endptr = NULL;
    long p = std::strtol(portstr.c_str(), &endptr, 10);
    if (endptr == portstr.c_str() || *endptr != '\0')
        return Logger::error("invalid port");
    if (p < 1 || p > 65535)
        return Logger::error("invalid port");
    std::string iface = v.substr(0, c);
    ListenAddress newAddr(iface, static_cast<int>(p));
    for (size_t i = 0; i < listenAddresses.size(); i++)
    {
        if (listenAddresses[i].getInterface() == newAddr.getInterface() &&
            listenAddresses[i].getPort() == newAddr.getPort())
            return Logger::error("duplicate listen address: " + v);
    }
    listenAddresses.push_back(newAddr);
    return true;
}

bool ServerConfig::setErrorPage(const std::vector<std::string> &values)
{
    if (values.size() < 2)
        return Logger::error("error_page requires at least error code and page path");
    std::string pagePath = values.back();
    for (size_t i = 0; i < values.size() - 1; i++)
    {
        char *endptr = NULL;
        long code = std::strtol(values[i].c_str(), &endptr, 10);
        if (endptr == values[i].c_str() || *endptr != '\0')
            return Logger::error("invalid error code: " + values[i]);
        if (code < 100 || code > 599)
            return Logger::error("error code must be between 100 and 599: " + values[i]);
        errorPages[static_cast<int>(code)] = pagePath;
    }
    return true;
}

void ServerConfig::addLocation(const LocationConfig &loc)
{
    locations.push_back(loc);
    if (locations.back().getRoot().empty())
        locations.back().setRoot(VectorString(1, root));
    if (locations.back().getIndexes().empty())
        locations.back().setIndexes(indexes);
}

// getters
int ServerConfig::getPort(size_t index) const
{
    return (index < listenAddresses.size()) ? listenAddresses[index].getPort() : -1;
}
std::string ServerConfig::getInterface(size_t index) const
{
    return (index < listenAddresses.size()) ? listenAddresses[index].getInterface() : "";
}
const std::vector<ListenAddress> &ServerConfig::getListenAddresses() const
{
    return listenAddresses;
}
bool ServerConfig::hasPort(int port) const
{
    for (size_t i = 0; i < listenAddresses.size(); i++)
    {
        if (listenAddresses[i].getPort() == port)
            return true;
    }
    return false;
}
std::vector<LocationConfig> &ServerConfig::getLocations()
{
    return locations;
}

const std::vector<LocationConfig> &ServerConfig::getLocations() const
{
    return locations;
}
VectorString ServerConfig::getIndexes() const
{
    return indexes;
}
std::string ServerConfig::getServerName(size_t index) const
{
    return (index < serverNames.size()) ? serverNames[index] : "";
}
const std::vector<std::string> &ServerConfig::getServerNames() const
{
    return serverNames;
}
bool ServerConfig::hasServerName(const std::string &name) const
{
    for (size_t i = 0; i < serverNames.size(); i++)
    {
        if (serverNames[i] == name)
            return true;
    }
    return false;
}
std::string ServerConfig::getRoot() const
{
    return root;
}

std::string ServerConfig::getClientMaxBody() const
{
    return clientMaxBodySize;
}

const std::map<int, std::string> &ServerConfig::getErrorPages() const
{
    return errorPages;
}

std::string ServerConfig::getErrorPage(int code) const
{
    std::map<int, std::string>::const_iterator it = errorPages.find(code);
    return (it != errorPages.end()) ? it->second : "";
}

bool ServerConfig::hasErrorPage(int code) const
{
    return errorPages.find(code) != errorPages.end();
}