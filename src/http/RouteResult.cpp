#include "RouteResult.hpp"

RouteResult::RouteResult()
    : statusCode(0),
      isRedirect(false),
      redirectUrl(""),
      pathRootUri(""),
      matchedPath(""),
      remainingPath(""),
      errorMessage(""),
      server(NULL),
      location(NULL),
      isCgiRequest(false),
      isUploadRequest(false),
      request(),
      handlerType(NOT_FOUND) {}

RouteResult::RouteResult(const RouteResult& other)
    : statusCode(other.statusCode),
      isRedirect(other.isRedirect),
      redirectUrl(other.redirectUrl),
      pathRootUri(other.pathRootUri),
      matchedPath(other.matchedPath),
      remainingPath(other.remainingPath),
      errorMessage(other.errorMessage),
      server(other.server),
      location(other.location),
      isCgiRequest(other.isCgiRequest),
      isUploadRequest(other.isUploadRequest),
      request(other.request),
      handlerType(other.handlerType) {}

RouteResult& RouteResult::operator=(const RouteResult& other) {
    if (this != &other) {
        statusCode      = other.statusCode;
        isRedirect      = other.isRedirect;
        redirectUrl     = other.redirectUrl;
        pathRootUri     = other.pathRootUri;
        matchedPath     = other.matchedPath;
        remainingPath   = other.remainingPath;
        errorMessage    = other.errorMessage;
        server          = other.server;
        location        = other.location;
        isCgiRequest    = other.isCgiRequest;
        isUploadRequest = other.isUploadRequest;
        request         = other.request;
        handlerType     = other.handlerType;
    }
    return *this;
}

RouteResult::~RouteResult() {}

// Setters
void RouteResult::setStatusCode(int code) {
    statusCode = code;
}

RouteResult& RouteResult::setRedirect(const String& url, int code) {
    isRedirect  = true;
    redirectUrl = url;
    statusCode  = code;
    if (code < 300 || code >= 400) {
        Logger::error("Invalid redirect status code: " + typeToString(code));
        statusCode = 301;
    }
    if (redirectUrl.empty()) {
        Logger::error("Empty redirect URL");
        isRedirect = false;
        statusCode = HTTP_INTERNAL_SERVER_ERROR;
        return *this;
    }
    return *this;
}

void RouteResult::setPathRootUri(const String& uri) {
    pathRootUri = uri;
}

void RouteResult::setMatchedPath(const String& path) {
    matchedPath = path;
}

void RouteResult::setRemainingPath(const String& path) {
    remainingPath = path;
}

void RouteResult::setErrorMessage(const String& message) {
    errorMessage = message;
}

void RouteResult::setServer(const ServerConfig* _server) {
    server = _server;
}

void RouteResult::setLocation(const LocationConfig* _location) {
    location = _location;
}

void RouteResult::setCgiRequest(bool isCgi) {
    isCgiRequest = isCgi;
}

void RouteResult::setUploadRequest(bool isUpload) {
    isUploadRequest = isUpload;
}

void RouteResult::setRequest(const HttpRequest& req) {
    request = req;
}

void RouteResult::setHandlerType(HandlerType type) {
    handlerType = type;
}

RouteResult& RouteResult::setCodeAndMessage(int code, const String& message) {
    statusCode   = code;
    errorMessage = message;
    return *this;
}

void RouteResult::setRemoteAddress(const String& address) {
    remoteAddress = address;
}

// Getters
int RouteResult::getStatusCode() const {
    return statusCode;
}

bool RouteResult::getIsRedirect() const {
    return isRedirect;
}

const String& RouteResult::getRedirectUrl() const {
    return redirectUrl;
}

const String& RouteResult::getPathRootUri() const {
    return pathRootUri;
}

const String& RouteResult::getMatchedPath() const {
    return matchedPath;
}

const String& RouteResult::getRemainingPath() const {
    return remainingPath;
}

const String& RouteResult::getErrorMessage() const {
    return errorMessage;
}

const ServerConfig* RouteResult::getServer() const {
    return server;
}

const LocationConfig* RouteResult::getLocation() const {
    return location;
}

bool RouteResult::getIsCgiRequest() const {
    return isCgiRequest;
}

bool RouteResult::getIsUploadRequest() const {
    return isUploadRequest;
}

const HttpRequest& RouteResult::getRequest() const {
    return request;
}

HandlerType RouteResult::getHandlerType() const {
    return handlerType;
}

const String& RouteResult::getRemoteAddress() const {
    return remoteAddress;
}
