#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <cstdlib>
#include <iostream>
#include <vector>
#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/RouteResult.hpp"
#include "../utils/Utils.hpp"
#include "HttpRequest.hpp"

class Router {
   public:
    Router();
    Router(int statusCode, String errorMessage);
    Router(const Router& other);
    Router& operator=(const Router& other);
    Router(const VectorServerConfig& servers, const HttpRequest& request);
    ~Router();

    RouteResult processRequest();

   private:
    const ServerConfig*   findServer() const;
    const ServerConfig*   getDefaultServer(int port) const;
    const LocationConfig* bestMatchLocation(const VectorLocationConfig& locations) const;
    String                resolveFilesystemPath(const LocationConfig* loc) const;
    bool                  checkBodySize(const LocationConfig& loc) const;
    bool                  isCgiRequest(const String& path, const LocationConfig& loc) const;
    void                  resolveCgiScriptAndPathInfo(const LocationConfig* loc, String& scriptPath, String& pathInfo) const;
    VectorServerConfig    _servers; // params from config
    HttpRequest           _request; // param from http request
};

#endif