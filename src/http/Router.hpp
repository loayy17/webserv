#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <cstdlib>
#include <iostream>
#include <vector>
#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
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
    const LocationConfig* bestMatchLocation(const VectorLocationConfig& locationsMatchServer) const;
    void                  processRequest();
    const ServerConfig*   findServer() const;
    String                resolveFilesystemPath() const;
    bool                  checkBodySize(const LocationConfig& location) const;
    bool                  isCgiRequest(const String& path, const LocationConfig& location) const;
    bool                  isUploadRequest(const String& method, const LocationConfig& location) const;
    const ServerConfig*   getDefaultServer(int port) const;

    bool getIsPathFound() const;
    bool getIsRedirect() const;
    int  getStatusCode() const;

    const LocationConfig* getLocation() const;
    const ServerConfig*   getServer() const;
    const String&         getPathRootUri() const;
    const String&         getMatchedPath() const;
    const String&         getRemainingPath() const;
    const String&         getRedirectUrl() const;
    const String&         getErrorMessage() const;
    const HttpRequest&    getRequest() const;

   private:
    VectorServerConfig    _servers;      // params from config
    HttpRequest           _request;      // param from http request
    bool                  isPathFound;   // is for checking if path of uri found in locations
    String                pathRootUri;   // the full path after combining root and uri
    String                matchedPath;   // the part of uri that matched with location path like /images
    String                remainingPath; // the part of uri that is after matched path like /photo.jpg
    const LocationConfig* matchLocation; // the location that matched with uri
    const ServerConfig*   matchServer;   // the server that matched with host and port
    String                redirectUrl;   // the URL to redirect to if isRedirect is true
    bool                  isRedirect;    // indicates if the request should be redirected
    int                   statusCode;    // HTTP status code for the response
    String                errorMessage;  // error message if any error occurs
};

#endif