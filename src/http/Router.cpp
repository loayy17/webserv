#include "Router.hpp"

// Constructors / Destructor
Router::Router() : _servers(), _request() {}
Router::Router(const VectorServerConfig& servers, const HttpRequest& request) : _servers(servers), _request(request) {}
Router::Router(const Router& other) : _servers(other._servers), _request(other._request) {}
Router& Router::operator=(const Router& other) {
    if (this != &other) {
        _servers = other._servers;
        _request = other._request;
    }
    return *this;
}
Router::~Router() {}

// Resolve CGI script and PATH_INFO by walking the URI path components
void Router::resolveCgiScriptAndPathInfo(const LocationConfig* loc, String& scriptPath, String& pathInfo) const {
    pathInfo.clear();

    if (!loc || !loc->hasCgi())
        return;

    // Get the base root for this location
    String root    = loc->getRoot();
    String locPath = normalizePath(loc->getPath());
    String uri     = normalizePath(_request.getUri());

    // Strip query string from URI
    size_t qpos = uri.find('?');
    if (qpos != String::npos)
        uri = uri.substr(0, qpos);

    // Get the part of the URI after the location prefix
    String rest = (locPath == "/") ? uri : (pathStartsWith(uri, locPath) ? uri.substr(locPath.length()) : uri);
    String accumulated;
    size_t pos = 0;
    while (pos < rest.length()) {
        // Find next slash
        size_t nextSlash = rest.find('/', pos == 0 && rest[0] == '/' ? 1 : pos);
        if (nextSlash == 0) {
            pos = 1;
            continue;
        } // skip leading slash
        String segment = (nextSlash == String::npos) ? rest.substr(pos) : rest.substr(pos, nextSlash - pos);
        if (segment.empty()) {
            pos = (nextSlash == String::npos) ? rest.length() : nextSlash + 1;
            continue;
        }

        if (accumulated.empty())
            accumulated = "/" + segment;
        else
            accumulated += "/" + segment;

        String candidate = joinPaths(root, accumulated);
        if (fileExists(candidate) && getFileType(candidate) == SINGLEFILE && isCgiRequest(candidate, *loc)) {
            scriptPath = candidate;
            // Everything after this segment is PATH_INFO
            size_t afterScript = (nextSlash == String::npos) ? rest.length() : nextSlash;
            if (afterScript < rest.length())
                pathInfo = rest.substr(afterScript);
            return;
        }
        pos = (nextSlash == String::npos) ? rest.length() : nextSlash + 1;
    }

    // Fallback: try the full resolved path
    scriptPath = joinPaths(root, rest.empty() ? "/" : rest);
}

// Main request processing
RouteResult Router::processRequest() {
    RouteResult result;
    result.setRequest(_request);

    // 1. Find server
    const ServerConfig* srv = findServer();
    if (!srv)
        return result.setCodeAndMessage(HTTP_BAD_REQUEST, "No matching server found");
    result.setServer(srv);

    // 2. Find location
    const LocationConfig* loc = bestMatchLocation(srv->getLocations());
    if (!loc)
        return result.setCodeAndMessage(HTTP_NOT_FOUND, "No matching location found");
    result.setLocation(loc);

    // 3. Redirect
    if (loc->getIsRedirect())
        return result.setRedirect(loc->getRedirectValue(), loc->getRedirectCode());

    // 4. Method check
    if (!isKeyInVector(_request.getMethod(), loc->getAllowedMethods()))
        return result.setCodeAndMessage(HTTP_METHOD_NOT_ALLOWED, "Method Not Allowed");

    // 5. Body size check
    if (_request.getContentLength() > 0 && !checkBodySize(*loc))
        return result.setCodeAndMessage(HTTP_PAYLOAD_TOO_LARGE, "Request Entity Too Large");

    // 6. CGI handling — check BEFORE upload so POST to .py/.php goes to CGI
    if (loc->hasCgi()) {
        // Walk the URI segments to find the CGI script and extract PATH_INFO
        String scriptPath, pathInfo;
        resolveCgiScriptAndPathInfo(loc, scriptPath, pathInfo);

        if (fileExists(scriptPath) && isCgiRequest(scriptPath, *loc)) {
            result.setPathRootUri(scriptPath);
            result.setRemainingPath(pathInfo);
            result.setMatchedPath(loc->getPath());
            result.setCgiRequest(true);
            result.setStatusCode(HTTP_OK);
            return result;
        }
    }

    // 7. Upload handling (POST/PUT to a location with upload_dir)
    if (!loc->getUploadDir().empty() && (_request.getMethod() == "POST" || _request.getMethod() == "PUT")) {
        result.setUploadRequest(true);
        result.setMatchedPath(loc->getPath());
        result.setStatusCode(HTTP_OK);
        return result;
    }
    // 8. Resolve filesystem path for static/directory
    String fsPath = resolveFilesystemPath(loc);
    if (!fileExists(fsPath)) {
        return result.setCodeAndMessage(HTTP_NOT_FOUND, "File not found");
    }
    // If path is a directory, try to resolve index file
    if (getFileType(fsPath) == DIRECTORY) {
        const VectorString& indexes    = loc->getIndexes();
        bool                foundIndex = false;
        for (size_t i = 0; i < indexes.size(); ++i) {
            String indexPath = joinPaths(fsPath, indexes[i]);
            if (fileExists(indexPath) && getFileType(indexPath) == SINGLEFILE) {
                fsPath     = indexPath;
                foundIndex = true;
                break;
            }
        }

        if (!foundIndex) {
            if (loc->getAutoIndex()) {
                result.setPathRootUri(fsPath);
                result.setMatchedPath(loc->getPath());
                result.setStatusCode(200);
                return result;
            }
            return result.setCodeAndMessage(HTTP_FORBIDDEN, "Forbidden");
        }
    }

    if (!fileExists(fsPath))
        return result.setCodeAndMessage(HTTP_NOT_FOUND, "File not found");
    result.setPathRootUri(fsPath);
    result.setMatchedPath(loc->getPath());

    // 9. Compute remaining path
    String remaining;
    if (_request.getUri().length() > result.getMatchedPath().length())
        remaining = _request.getUri().substr(result.getMatchedPath().length());
    result.setRemainingPath(remaining);
    result.setStatusCode(HTTP_OK);
    return result;
}

// Server lookup
const ServerConfig* Router::findServer() const {
    int    port = _request.getPort();
    String host = _request.getHost();

    for (size_t i = 0; i < _servers.size(); ++i) {
        if (_servers[i].hasPort(port) && _servers[i].hasServerName(host))
            return &_servers[i];
    }
    return getDefaultServer(port);
}

const ServerConfig* Router::getDefaultServer(int port) const {
    for (size_t i = 0; i < _servers.size(); ++i)
        if (_servers[i].hasPort(port))
            return &_servers[i];
    return NULL;
}

// Best matching location
const LocationConfig* Router::bestMatchLocation(const VectorLocationConfig& locations) const {
    String                uri     = normalizePath(_request.getUri());
    const LocationConfig* best    = NULL;
    size_t                bestLen = 0;

    for (size_t i = 0; i < locations.size(); ++i) {
        String path = normalizePath(locations[i].getPath());
        if (pathStartsWith(uri, path) && path.length() > bestLen) {
            best    = &locations[i];
            bestLen = path.length();
        }
    }
    return best;
}

// Resolve filesystem path
String Router::resolveFilesystemPath(const LocationConfig* loc) const {
    if (!loc)
        return "";

    String root    = loc->getRoot();
    String locPath = normalizePath(loc->getPath());

    String uri = normalizePath(_request.getUri());
    // compute the part of URI after the location prefix
    // firstgo topath start with rest for /login and location /login rest is empty becasue when location and uri pic.jpg rest is /pic.jpg
    String rest;
    if (locPath == "/") {
        rest = uri;
    } else if (pathStartsWith(uri, locPath)) {
        rest = uri.substr(locPath.length());
    } else {
        rest = uri;
    }
    if (rest.empty() || rest[0] != '/')
        rest = "/" + rest;
    String fullPath = joinPaths(root, locPath);
    return joinPaths(fullPath, rest);
}

// Body size
bool Router::checkBodySize(const LocationConfig& loc) const {
    String maxBody = loc.getClientMaxBody();
    return maxBody.empty() || _request.getContentLength() <= convertMaxBodySize(maxBody);
}

// Check CGI request
bool Router::isCgiRequest(const String& path, const LocationConfig& loc) const {
    if (!loc.hasCgi())
        return false;

    String name, ext;
    if (!splitByChar(path, name, ext, '.', true))
        return false;

    return !loc.getCgiInterpreter("." + ext).empty();
}
