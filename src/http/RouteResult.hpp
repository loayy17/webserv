#ifndef ROUTE_RESULT_HPP
#define ROUTE_RESULT_HPP
#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../utils/Enums.hpp"
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
    void setHandlerType(HandlerType type);
    void setRemoteAddress(const String& address);

    void setBodyOffset(size_t offset);
    void setBodyLength(size_t length);
    void setRequestSize(size_t size);
    void setClientBuffer(const String* buffer);
    void setDechunkedBody(const String& body);
    void setUseDechunked(bool use);

    // Getters
    int                   getStatusCode() const;
    bool                  getIsRedirect() const;
    const String&         getRedirectUrl() const;
    const String&         getPathRootUri() const;
    const String&         getMatchedPath() const;
    const String&         getRemainingPath() const;
    const String&         getErrorMessage() const;
    const ServerConfig*   getServer() const;
    const LocationConfig* getLocation() const;
    bool                  getIsCgiRequest() const;
    bool                  getIsUploadRequest() const;
    const HttpRequest&    getRequest() const;
    HandlerType           getHandlerType() const;
    const String&         getRemoteAddress() const;

    size_t                getBodyOffset() const;
    size_t                getBodyLength() const;
    size_t                getRequestSize() const;
    const String*         getClientBuffer() const;
    const String&         getDechunkedBody() const;
    bool                  getUseDechunked() const;

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
    HandlerType           handlerType;
    String                remoteAddress;

    size_t                bodyOffset;
    size_t                bodyLength;
    size_t                requestSize;
    const String*         clientBuffer;
    String                dechunkedBody;
    bool                  useDechunked;
};
#endif