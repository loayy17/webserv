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
Router::Router(int _statusCode, String errorMessage)
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
      statusCode(_statusCode),
      errorMessage(errorMessage) {}

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
        _servers      = other._servers;
        _request      = other._request;
        isPathFound   = other.isPathFound;
        pathRootUri   = other.pathRootUri;
        matchedPath   = other.matchedPath;
        remainingPath = other.remainingPath;
        matchLocation = other.matchLocation;
        matchServer   = other.matchServer;
        redirectUrl   = other.redirectUrl;
        isRedirect    = other.isRedirect;
        statusCode    = other.statusCode;
        errorMessage  = other.errorMessage;
    }
    return *this;
}

Router::Router(const VectorServerConfig& servers, const HttpRequest& request)
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
    if (!isKeyInVector(_request.getMethod(), matchLocation->getAllowedMethods())) {
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

    // 6. resolve filesystem path
    pathRootUri = resolveFilesystemPath();
    if (_request.getUri().length() > matchLocation->getPath().length()) {
        remainingPath = _request.getUri().substr(matchLocation->getPath().length());
    }

    std::cout << "Resolved path: " << pathRootUri << std::endl;
    statusCode = 200;
    return;
}

const ServerConfig* Router::findServer() const {
    int    requestPort = _request.getPort();
    String requestHost = _request.getHost();

    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i].hasPort(requestPort)) {
            if (_servers[i].hasServerName(requestHost)) {
                return &_servers[i];
            }
        }
    }
    return getDefaultServer(requestPort);
}

const ServerConfig* Router::getDefaultServer(int port) const {
    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i].hasPort(port)) {
            return &_servers[i];
        }
    }
    return NULL;
}

const LocationConfig* Router::bestMatchLocation(const VectorLocationConfig& locationsMatchServer) const {
    String                normalizedUri = normalizePath(_request.getUri());
    const LocationConfig* bestMatch     = NULL;

    size_t bestMatchLength = 0;
    for (size_t i = 0; i < locationsMatchServer.size(); i++) {
        size_t locPathLength = locationsMatchServer[i].getPath().length();
        String locPath       = normalizePath(locationsMatchServer[i].getPath());
        if (pathStartsWith(normalizedUri, locPath)) {
            if (locPathLength > bestMatchLength) {
                bestMatchLength = locPathLength;
                bestMatch       = &locationsMatchServer[i];
            }
        }
    }

    return bestMatch;
}

String Router::resolveFilesystemPath() const {
    if (!matchLocation)
        return "";

    String root    = matchLocation->getRoot();
    String locPath = normalizePath(matchLocation->getPath());
    String uri     = normalizePath(_request.getUri());

    if (!root.empty() && root[0] == '/')
        root = "." + root;

    String rest;
    if (locPath == "/")
        rest = uri;
    else if (pathStartsWith(uri, locPath))
        rest = uri.substr(locPath.length());
    else
        rest = uri;

    if (rest.empty())
        rest = "/";

    return joinPaths(root, rest);
}

bool Router::checkBodySize(const LocationConfig& location) const {
    String maxBodyStr = location.getClientMaxBody();
    if (maxBodyStr.empty())
        return true;
    size_t maxBody = convertMaxBodySize(maxBodyStr);
    return _request.getContentLength() <= maxBody;
}

bool Router::isCgiRequest(const String& path, const LocationConfig& location) const {
    MapString cgiPass = location.getCgiPass();
    for (MapString::const_iterator it = cgiPass.begin(); it != cgiPass.end(); ++it) {
        std::cout << "CGI Pass: " << it->first << " => " << it->second << std::endl;
    }
    std::cout << "Checking if cgipass map is empty: " << location.getCgiPass().empty() << std::endl;
    if (!location.hasCgi())
        return false;
    std::cout << "Checking CGI for path: " << path << std::endl;
    size_t dotPos = path.rfind('.');
    std::cout << "Dot position: " << dotPos << std::endl;
    if (dotPos == String::npos)
        return false;

    String extension = path.substr(dotPos);
    std::cout << "Extension: " << extension << std::endl;
    std::cout << "Has CGI for extension: " << location.hasCgi() << std::endl;
    std::cout << "CGI Interpreter: " << location.getCgiInterpreter(extension) << std::endl;

    return !location.getCgiInterpreter(extension).empty();
}

bool Router::isUploadRequest(const String& method, const LocationConfig& location) const {
    return !location.getUploadDir().empty() && method == "POST";
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
const String& Router::getPathRootUri() const {
    return pathRootUri;
}
const String& Router::getMatchedPath() const {
    return matchedPath;
}
const String& Router::getRemainingPath() const {
    return remainingPath;
}
const String& Router::getRedirectUrl() const {
    return redirectUrl;
}
const String& Router::getErrorMessage() const {
    return errorMessage;
}

const HttpRequest& Router::getRequest() const {
    return _request;
}