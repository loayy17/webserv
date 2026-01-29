#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <cstdlib>
#include <iostream>
#include <vector>
#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class Router {
   public:
    Router();
    Router(const Router& other);
    Router& operator=(const Router& other);
    Router(const std::vector<ServerConfig>& servers, const HttpRequest& request);
    ~Router();
    const LocationConfig* bestMatchLocation(const std::vector<LocationConfig>& locationsMatchServer) const;
    void                  processRequest();
    const ServerConfig*   findServer() const;
    std::string           resolveFilesystemPath() const;
    bool                  checkBodySize(const LocationConfig& location) const;
    bool                  isCgiRequest(const std::string& path, const LocationConfig& location) const;
    bool                  isUploadRequest(const std::string& method, const LocationConfig& location) const;
    const ServerConfig*   getDefaultServer(int port) const;

    bool getIsPathFound() const;
    bool getIsRedirect() const;
    int  getStatusCode() const;

    const LocationConfig* getLocation() const;
    const ServerConfig*   getServer() const;
    const std::string&    getPathRootUri() const;
    const std::string&    getMatchedPath() const;
    const std::string&    getRemainingPath() const;
    const std::string&    getRedirectUrl() const;
    const std::string&    getErrorMessage() const;

   private:
    std::vector<ServerConfig> _servers;      // params from config
    HttpRequest               _request;      // param from http request
    bool                      isPathFound;   // is for checking if path of uri found in locations
    std::string               pathRootUri;   // the full path after combining root and uri
    std::string               matchedPath;   // the part of uri that matched with location path like /images
    std::string               remainingPath; // the part of uri that is after matched path like /photo.jpg
    const LocationConfig*     matchLocation; // the location that matched with uri
    const ServerConfig*       matchServer;   // the server that matched with host and port
    std::string               redirectUrl;   // the URL to redirect to if isRedirect is true
    bool                      isRedirect;    // indicates if the request should be redirected
    int                       statusCode;    // HTTP status code for the response
    std::string               errorMessage;  // error message if any error occurs
};

#endif