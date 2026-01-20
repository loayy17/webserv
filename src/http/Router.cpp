#include "Router.hpp"
#include <cstdlib>
#include <algorithm>

RouteResult::RouteResult()
    : found(false),
      location(NULL),
      server(NULL),
      resolvedPath(""),
      matchedPath(""),
      remainingPath(""),
      isRedirect(false),
      redirectUrl(""),
      statusCode(200),
      errorMessage("") {}

Router::Router() {}

Router::Router(const std::vector<ServerConfig>& servers) : _servers(servers) {}

Router::~Router() {}

void Router::init(const std::vector<ServerConfig>& servers) {
    _servers = servers;
}

RouteResult Router::route(const HttpRequest& request, int port) const {
    RouteResult result;

    const ServerConfig* server = findServer(request.getHost(), port);
    if (!server) {
        server = getDefaultServer(port);
    }

    if (!server) {
        result.found = false;
        result.statusCode = 500;
        result.errorMessage = "No server configured for this port";
        return result;
    }
    result.server = server;

    std::string uri = request.getUri();
    const LocationConfig* location = findLocation(uri, *server);

    if (!location) {
        result.found = false;
        result.statusCode = 404;
        result.errorMessage = "No location matches the requested URI";
        return result;
    }
    result.location = location;
    result.found = true;
    result.matchedPath = location->getPath();

    if (!location->getRedirect().empty()) {
        result.isRedirect = true;
        result.redirectUrl = location->getRedirect();
        result.statusCode = 301; // Permanent redirect
        return result;
    }

    if (!isMethodAllowed(request.getMethod(), *location)) {
        result.statusCode = 405;
        result.errorMessage = "Method Not Allowed";
        return result;
    }

    if (request.getContentLength() > 0) {
        if (!checkBodySize(request.getContentLength(), *location)) {
            result.statusCode = 413;
            result.errorMessage = "Request Entity Too Large";
            return result;
        }
    }

    result.resolvedPath = resolvePath(uri, *location);

    if (uri.length() > location->getPath().length()) {
        result.remainingPath = uri.substr(location->getPath().length());
    }

    result.statusCode = 200;
    return result;
}

const ServerConfig* Router::findServer(const std::string& hostHeader, int port) const {
    std::string host = extractHost(hostHeader);

    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i].getPort() == port) {
            if (!_servers[i].getServerName().empty() && 
                _servers[i].getServerName() == host) {
                return &_servers[i];
            }
        }
    }

    return getDefaultServer(port);
}

const ServerConfig* Router::getDefaultServer(int port) const {
    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i].getPort() == port) {
            return &_servers[i];
        }
    }
    return NULL;
}

std::string Router::extractHost(const std::string& hostHeader) const {
    size_t colonPos = hostHeader.find(':');
    if (colonPos != std::string::npos) {
        return hostHeader.substr(0, colonPos);
    }
    return hostHeader;
}

const LocationConfig* Router::findLocation(const std::string& uri, const ServerConfig& server) const {
    const LocationConfig* bestMatch = NULL;
    size_t bestMatchLength = 0;

    std::string normalizedUri = normalizePath(uri);
    const std::vector<LocationConfig>& locations = server.getLocations();

    for (size_t i = 0; i < locations.size(); i++) {
        const std::string& locPath = locations[i].getPath();

        if (pathStartsWith(normalizedUri, locPath)) {
            if (locPath.length() > bestMatchLength) {
                bestMatchLength = locPath.length();
                bestMatch = &locations[i];
            }
        }
    }

    return bestMatch;
}

bool Router::pathStartsWith(const std::string& path, const std::string& prefix) const {
    if (prefix.length() > path.length()) {
        return false;
    }

    if (path.compare(0, prefix.length(), prefix) != 0) {
        return false;
    }

    // For non-root locations, ensure we match at directory boundary
    // e.g., /images should match /images/cat.jpg but not /imagesbackup/cat.jpg
    if (prefix != "/" && prefix.length() < path.length()) {
        char nextChar = path[prefix.length()];
        if (nextChar != '/' && nextChar != '?' && nextChar != '#') {
            return false;
        }
    }

    return true;
}

std::string Router::normalizePath(const std::string& path) const {
    std::string normalized = path;

    size_t queryPos = normalized.find('?');
    if (queryPos != std::string::npos) {
        normalized = normalized.substr(0, queryPos);
    }

    size_t fragPos = normalized.find('#');
    if (fragPos != std::string::npos) {
        normalized = normalized.substr(0, fragPos);
    }

    if (normalized.empty() || normalized[0] != '/') {
        normalized = "/" + normalized;
    }

    std::string result;
    bool lastWasSlash = false;
    for (size_t i = 0; i < normalized.length(); i++) {
        if (normalized[i] == '/') {
            if (!lastWasSlash) {
                result += normalized[i];
                lastWasSlash = true;
            }
        } else {
            result += normalized[i];
            lastWasSlash = false;
        }
    }

    return result;
}

bool Router::isMethodAllowed(const std::string& method, const LocationConfig& location) const {
    const std::vector<std::string>& allowed = location.getAllowedMethods();

    for (size_t i = 0; i < allowed.size(); i++) {
        if (allowed[i] == method) {
            return true;
        }
    }
    return false;
}

std::string Router::resolvePath(const std::string& uri, const LocationConfig& location) const {
    std::string root = location.getRoot();
    std::string locPath = location.getPath();
    std::string normalizedUri = normalizePath(uri);

    if (!root.empty() && root[root.length() - 1] == '/') {
        root = root.substr(0, root.length() - 1);
    }

    std::string pathAfterLocation;
    if (normalizedUri.length() > locPath.length()) {
        pathAfterLocation = normalizedUri.substr(locPath.length());
    } else if (normalizedUri == locPath) {
        pathAfterLocation = "";
    } else {
        pathAfterLocation = normalizedUri;
    }

    if (!pathAfterLocation.empty() && pathAfterLocation[0] != '/') {
        pathAfterLocation = "/" + pathAfterLocation;
    }

    if (locPath == "/") {
        return root + normalizedUri;
    }

    return root + pathAfterLocation;
}

bool Router::checkBodySize(size_t contentLength, const LocationConfig& location) const {
    std::string maxBodyStr = location.getClientMaxBody();
    if (maxBodyStr.empty()) {
        return true;
    }

    size_t maxBody = parseBodySize(maxBodyStr);
    return contentLength <= maxBody;
}

size_t Router::parseBodySize(const std::string& sizeStr) const {
    if (sizeStr.empty()) {
        return 1048576;
    }

    size_t len = sizeStr.length();
    char suffix = sizeStr[len - 1];
    std::string numPart;

    if (suffix == 'K' || suffix == 'k' || 
        suffix == 'M' || suffix == 'm' || 
        suffix == 'G' || suffix == 'g') {
        numPart = sizeStr.substr(0, len - 1);
    } else {
        numPart = sizeStr;
        suffix = 'B';
    }

    size_t value = static_cast<size_t>(std::atol(numPart.c_str()));

    switch (suffix) {
        case 'K':
        case 'k':
            return value * 1024;
        case 'M':
        case 'm':
            return value * 1024 * 1024;
        case 'G':
        case 'g':
            return value * 1024 * 1024 * 1024;
        default:
            return value;
    }
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

    return path.compare(path.length() - extension.length(), 
                        extension.length(), extension) == 0;
}

bool Router::isUploadRequest(const std::string& method, const LocationConfig& location) const {
    return location.getUploadEnabled() && method == "POST";
}
