#ifndef ROUTE_RESULT_HPP
#define ROUTE_RESULT_HPP
#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../utils/Types.hpp"
#include "HttpRequest.hpp"

class RouteResult {
   public:
    RouteResult();
    RouteResult(const RouteResult& other);
    RouteResult& operator=(const RouteResult& other);
    ~RouteResult();

    RouteResult& setCodeAndMessage(int code, const String& message);
    RouteResult& setRedirect(const String& url, int code);
    // Setters
    void setStatusCode(int code);
    void setPathRootUri(const String& uri);
    void setMatchedPath(const String& path);
    void setRemainingPath(const String& path);
    void setErrorMessage(const String& message);
    void setServer(const ServerConfig* _server);
    void setLocation(const LocationConfig* _location);
    void setCgiRequest(bool isCgi);
    void setUploadRequest(bool isUpload);
    void setRequest(const HttpRequest& req);

    // Getters
    int                   getStatusCode() const;
    bool                  getIsRedirect() const;
    String                getRedirectUrl() const;
    String                getPathRootUri() const;
    String                getMatchedPath() const;
    String                getRemainingPath() const;
    String                getErrorMessage() const;
    const ServerConfig*   getServer() const;
    const LocationConfig* getLocation() const;
    bool                  getIsCgiRequest() const;
    bool                  getIsUploadRequest() const;
    const HttpRequest&    getRequest() const;

   private:
    int                   statusCode;
    bool                  isRedirect;
    String                redirectUrl;
    String                pathRootUri;
    String                matchedPath;
    String                remainingPath;
    String                errorMessage;
    const ServerConfig*   server;
    const LocationConfig* location;
    bool                  isCgiRequest;
    bool                  isUploadRequest;
    HttpRequest           request;
};
#endif