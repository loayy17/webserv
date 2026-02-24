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
void Router::resolveCgiScriptAndPathInfo(const LocationConfig* loc, String& scriptPath, String& pathInfo) const {
    scriptPath.clear();
    pathInfo.clear();

    if (!loc || !loc->hasCgi())
        return;

    const String root    = loc->getRoot();
    const String locPath = normalizePath(loc->getPath());
    const String uri     = normalizePath(_request.getUri());
    const String rest    = getUriRemainder(uri, locPath);

    String directFile = joinPaths(root, rest);
    if (fileExists(directFile) && getFileType(directFile) == SINGLEFILE && isCgiRequest(directFile, *loc)) {
        scriptPath = directFile;
        return;
    }

    VectorString segments;
    splitByString(rest, segments, "/");
    
    for (size_t i = segments.size(); i > 0; --i) {
        String accumulated;
        for (size_t k = 0; k < i; ++k) {
            if (!segments[k].empty())
                accumulated += "/" + segments[k];
        }
        
        if (accumulated.empty()) continue;
        
        String candidate = joinPaths(root, accumulated);
        if (fileExists(candidate) && getFileType(candidate) == SINGLEFILE && isCgiRequest(candidate, *loc)) {
            scriptPath = candidate;
            if (rest.size() > accumulated.size()) {
                pathInfo = rest.substr(accumulated.size());
                if (pathInfo.empty() || pathInfo[0] != '/') {
                    pathInfo = "/" + pathInfo;
                }
            } else if (rest.size() == accumulated.size()) {
                pathInfo.clear();
            }
            return;
        }
    }
    // fallback
    scriptPath = directFile;
}

// Main request processing
RouteResult Router::processRequest() {
    RouteResult result;
    result.setRequest(_request);

    // 1. Find server
    const ServerConfig* srv = findServer();
    if (!srv)
        return result.setCodeAndMessage(HTTP_BAD_REQUEST, getHttpStatusMessage(HTTP_BAD_REQUEST));
    result.setServer(srv);

    // 2. Find location
    const LocationConfig* loc = bestMatchLocation(srv->getLocations());
    if (!loc)
        return result.setCodeAndMessage(HTTP_NOT_FOUND, getHttpStatusMessage(HTTP_NOT_FOUND));
    result.setLocation(loc);
    result.setMatchedPath(loc->getPath());

    // 3. Redirect
    if (loc->getIsRedirect())
        return result.setRedirect(loc->getRedirectValue(), loc->getRedirectCode());

    // 4. Method check
    String methodToCheck = _request.getMethod();
    // if (methodToCheck == "HEAD")
    //     methodToCheck = "GET";
    if (!isKeyInVector(methodToCheck, loc->getAllowedMethods()))
        return result.setCodeAndMessage(HTTP_METHOD_NOT_ALLOWED, getHttpStatusMessage(HTTP_METHOD_NOT_ALLOWED));

    // 5. CGI handling
    if (loc->hasCgi()) {
        String scriptPath, pathInfo;
        resolveCgiScriptAndPathInfo(loc, scriptPath, pathInfo);

        if (isCgiRequest(scriptPath, *loc)) {
            result.setPathRootUri(scriptPath);
            result.setRemainingPath(pathInfo);
            result.setCgiRequest(true);
            result.setHandlerType(CGI);
            result.setStatusCode(HTTP_OK);
            return result;
        }
    }

    // 6. Upload handling (POST/PUT to a location with upload_dir)
    if (!loc->getUploadDir().empty() && (_request.getMethod() == "POST" || _request.getMethod() == "PUT")) {
        result.setUploadRequest(true);
        result.setHandlerType(UPLOAD);
        result.setStatusCode(HTTP_OK);
        return result;
    }

    // 7. Resolve filesystem path for static/directory
    String fsPath = resolveFilesystemPath(loc);
    if (!fileExists(fsPath))
        return result.setCodeAndMessage(HTTP_NOT_FOUND, getHttpStatusMessage(HTTP_NOT_FOUND));

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
                result.setHandlerType(DIRECTORY_LISTING);
                result.setStatusCode(HTTP_OK);
                return result;
            }
            return result.setCodeAndMessage(HTTP_NOT_FOUND, getHttpStatusMessage(HTTP_NOT_FOUND));
        }

        // If the resolved index file is a CGI script, serve it as CGI
        if (loc->hasCgi() && isCgiRequest(fsPath, *loc)) {
            result.setPathRootUri(fsPath);
            result.setRemainingPath("");
            result.setCgiRequest(true);
            result.setHandlerType(CGI);
            result.setStatusCode(HTTP_OK);
            return result;
        }
    }

    if (!fileExists(fsPath))
        return result.setCodeAndMessage(HTTP_NOT_FOUND, getHttpStatusMessage(HTTP_NOT_FOUND));
    result.setPathRootUri(fsPath);

    // 8. Determine handler type based on method and file type
    String method = _request.getMethod();
    if (method == "DELETE") {
        result.setHandlerType(DELETE_FILE);
    } else if (method == "GET" || method == "HEAD") {
        result.setHandlerType(STATIC);
    } else {
        result.setHandlerType(NOT_FOUND);
    }

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

    String root    = loc->getRoot();                   // ./www
    String locPath = normalizePath(loc->getPath());    // path for location like /uploads
    String uri     = normalizePath(_request.getUri()); // actual request URI like /uploads/file.txt
    String rest    = getUriRemainder(uri, locPath);    // the part of URI after location path
    return joinPaths(root, rest);
}

// Check CGI request
bool Router::isCgiRequest(const String& path, const LocationConfig& loc) const {
    if (!loc.hasCgi())
        return false;

    String name, ext;
    if (!splitByChar(path, name, ext, '.', true))
        return false;

    return !loc.getCgiInterpreter("." + toLowerWords(ext)).empty();
}
