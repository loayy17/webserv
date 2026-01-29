#include "Router.hpp"

Router::Router() 
    : _servers(),
      _request(),
      isPathFound(false),
      pathRootUri(""),
      matchedPath(""),
      remainingPath(""),
      matchLocation(NULL),
      matchServer(NULL),
      redirectUrl(""),
      isRedirect(false),
      statusCode(0),
      errorMessage("") {}

Router::Router(const Router& other)
    : _servers(other._servers),
    _request(other._request),
    isPathFound(other.isPathFound),
    pathRootUri(other.pathRootUri),
    matchedPath(other.matchedPath),
    remainingPath(other.remainingPath),
    matchLocation(other.matchLocation),
    matchServer(other.matchServer),
    redirectUrl(other.redirectUrl),
    isRedirect(other.isRedirect),
    statusCode(other.statusCode),
    errorMessage(other.errorMessage) {}

Router& Router::operator=(const Router& other) {
    if (this != &other) {
        _servers       = other._servers;
        _request       = other._request;
        isPathFound    = other.isPathFound;
        pathRootUri    = other.pathRootUri;
        matchedPath    = other.matchedPath;
        remainingPath  = other.remainingPath;
        matchLocation  = other.matchLocation;
        matchServer    = other.matchServer;
        redirectUrl    = other.redirectUrl;
        isRedirect     = other.isRedirect;
        statusCode     = other.statusCode;
        errorMessage   = other.errorMessage;
    }
    return *this;
}

Router::Router(const std::vector<ServerConfig>& servers, const HttpRequest& request)
    : _servers(servers),
      _request(request),
      isPathFound(false),
      pathRootUri(""),
      matchedPath(""),
      remainingPath(""),
      matchLocation(NULL),
      matchServer(NULL),
      redirectUrl(""),
      isRedirect(false),
      statusCode(0),
      errorMessage("") {}

Router::~Router() {
    _servers.clear();
}

void Router::processRequest() {
    // TODO: client request port validation
    // TODO: check if must server has unique port
    // steps:
    // 1. find server based on Host header and port from request
    matchServer = findServer();
    if (!matchServer) {
        isPathFound  = false;
        statusCode   = 500;
        errorMessage = "No server configured for this port";
        return;
    }

    // 2. find best matching location with match server
    matchLocation = bestMatchLocation(matchServer->getLocations());
    if (!matchLocation) {
        isPathFound  = false;
        statusCode   = 404;
        errorMessage = "No location matches the requested URI";
        return;
    }
    // if matchLocation found
    // 3. check redirect
    isPathFound = true;
    matchedPath = matchLocation->getPath();
    if (!matchLocation->getRedirect().empty()) {
        isRedirect  = true;
        redirectUrl = matchLocation->getRedirect();
        statusCode  = 301;
        return;
    }

    // 4. check if method allowed in location
    if (!isStringInVector(_request.getMethod(), matchLocation->getAllowedMethods())) {
        statusCode   = 405;
        errorMessage = "Method Not Allowed";
        return;
    }

    // 5. check body size if is less than client_max_body_size from conf file
    if (_request.getContentLength() > 0) {
        if (!checkBodySize(*matchLocation)) {
            statusCode   = 413;
            errorMessage = "Request Entity Too Large";
            return;
        }
    }
    pathRootUri = resolveFilesystemPath();

    if (_request.getUri().length() > matchLocation->getPath().length()) {
        remainingPath = _request.getUri().substr(matchLocation->getPath().length());
    }
    statusCode = 200;
    return;
}

const ServerConfig* Router::findServer() const {
    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i].getPort() == _request.getPort()) {
            if (_servers[i].getServerName() == _request.getHost()) {
                return &_servers[i];
            }
        }
    }
    return getDefaultServer(_request.getPort());
}

const ServerConfig* Router::getDefaultServer(int port) const {
    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i].getPort() == port) {
            return &_servers[i];
        }
    }
    return NULL;
}

const LocationConfig* Router::bestMatchLocation(const std::vector<LocationConfig>& locationsMatchServer) const {
    std::string           normalizedUri = normalizePath(_request.getUri());
    const LocationConfig* bestMatch     = NULL;

    size_t bestMatchLength = 0;
    for (size_t i = 0; i < locationsMatchServer.size(); i++) {
        size_t locPathLength = locationsMatchServer[i].getPath().length();
        if (pathStartsWith(normalizedUri, locationsMatchServer[i].getPath())) {
            if (locPathLength > bestMatchLength) {
                bestMatchLength = locPathLength;
                bestMatch       = &locationsMatchServer[i];
            }
        }
    }

    return bestMatch;
}

std::string Router::resolveFilesystemPath() const {
    std::string root    = matchLocation->getRoot();
    std::string locPath = matchLocation->getPath();
    std::string uri     = normalizePath(_request.getUri());
    if (locPath == "/")
        return root + uri;

    std::string rest = uri.substr(locPath.length());
    if (!rest.empty() && rest[0] != '/')
        rest = "/" + rest;

    return root + rest;
}

bool Router::checkBodySize(const LocationConfig& location) const {
    std::string maxBodyStr = location.getClientMaxBody();
    if (maxBodyStr.empty())
        return true;
    size_t maxBody = convertMaxBodySize(maxBodyStr);
    return _request.getContentLength() <= maxBody;
}

bool Router::isCgiRequest(const std::string& path, const LocationConfig& location) const {
    if (!location.getCgiEnabled()) {
        return false;
    }

    std::string extension = location.getCgiExtension();
    if (extension.empty()) {
        return false;
    }

    if (path.length() < extension.length()) {
        return false;
    }

    return path.compare(path.length() - extension.length(), extension.length(), extension) == 0;
}

bool Router::isUploadRequest(const std::string& method, const LocationConfig& location) const {
    return location.getUploadEnabled() && method == "POST";
}

// Getters
bool Router::getIsPathFound() const {
    return isPathFound;
}
bool Router::getIsRedirect() const {
    return isRedirect;
}
int Router::getStatusCode() const {
    return statusCode;
}

const LocationConfig* Router::getLocation() const {
    return matchLocation;
}
const ServerConfig* Router::getServer() const {
    return matchServer;
}
const std::string& Router::getPathRootUri() const {
    return pathRootUri;
}
const std::string& Router::getMatchedPath() const {
    return matchedPath;
}
const std::string& Router::getRemainingPath() const {
    return remainingPath;
}
const std::string& Router::getRedirectUrl() const {
    return redirectUrl;
}
const std::string& Router::getErrorMessage() const {
    return errorMessage;
}