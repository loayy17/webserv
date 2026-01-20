#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <string>
#include <vector>
#include "../config/ServerConfig.hpp"
#include "../config/LocationConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

struct RouteResult {
    bool                found;
    const LocationConfig* location;
    const ServerConfig*   server;
    std::string         resolvedPath;
    std::string         matchedPath;
    std::string         remainingPath;
    bool                isRedirect;
    std::string         redirectUrl;
    int                 statusCode;
    std::string         errorMessage;

    RouteResult();
};

class Router {
   public:
    Router();
    Router(const std::vector<ServerConfig>& servers);
    ~Router();

    void init(const std::vector<ServerConfig>& servers);
    RouteResult route(const HttpRequest& request, int port) const;
    const ServerConfig* findServer(const std::string& hostHeader, int port) const;
    const LocationConfig* findLocation(const std::string& uri, const ServerConfig& server) const;
    bool isMethodAllowed(const std::string& method, const LocationConfig& location) const;
    std::string resolvePath(const std::string& uri, const LocationConfig& location) const;
    bool checkBodySize(size_t contentLength, const LocationConfig& location) const;
    size_t parseBodySize(const std::string& sizeStr) const;
    bool isCgiRequest(const std::string& path, const LocationConfig& location) const;
    bool isUploadRequest(const std::string& method, const LocationConfig& location) const;
    const ServerConfig* getDefaultServer(int port) const;

   private:
    std::vector<ServerConfig> _servers;
    std::string extractHost(const std::string& hostHeader) const;
    std::string normalizePath(const std::string& path) const;
    bool        pathStartsWith(const std::string& path, const std::string& prefix) const;
};

#endif