#include "CgiHandler.hpp"
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"

CgiHandler::CgiHandler() : _cgi(NULL) {}
CgiHandler::CgiHandler(CgiProcess& cgi) : _cgi(&cgi) {}
CgiHandler::CgiHandler(const CgiHandler& other) : IHandler(), _cgi(other._cgi) {}
CgiHandler& CgiHandler::operator=(const CgiHandler& other) {
    if (this != &other)
        _cgi = other._cgi;
    return *this;
}
CgiHandler::~CgiHandler() {}

// ─── IHandler interface ──────────────────────────────────────────────────────

bool CgiHandler::handle(const RouteResult& resultRouter, HttpResponse& response) const {
    return handle(resultRouter, response, VectorInt());
}
bool CgiHandler::handle(const RouteResult& resultRouter, HttpResponse& response, const VectorInt& openFds) const {
    (void)response;

    if (!_cgi)
        return false;

    const LocationConfig* loc = resultRouter.getLocation();
    if (!loc || !loc->hasCgi())
        return false;
    String scriptPath = resultRouter.getPathRootUri();
    String extension  = extractFileExtension(scriptPath);

    if (extension.empty())
        return Logger::error("Failed to parse CGI extension");

    String interpreter = loc->getCgiInterpreter(extension);
    int    parentToChild[2];
    int    childToParent[2];
    if (pipe(parentToChild) == -1)
        return Logger::error("CGI pipe parent->child failed");
    if (pipe(childToParent) == -1) {
        close(parentToChild[0]);
        close(parentToChild[1]);
        return Logger::error("CGI pipe child->parent failed");
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(parentToChild[0]);
        close(parentToChild[1]);
        close(childToParent[0]);
        close(childToParent[1]);
        return Logger::error("CGI fork failed");
    }

    String scriptDir = extractDirectoryFromPath(scriptPath);

    if (pid == 0) {
        if (close(parentToChild[1]) == -1) {
            perror("CGI close parentToChild[1] failed");
            _exit(1);
        }
        if (close(childToParent[0]) == -1) {
            perror("CGI close childToParent[0] failed");
            _exit(1);
        }
        if (dup2(parentToChild[0], STDIN_FILENO) == -1) {
            perror("CGI dup2 parentToChild[0] failed");
            _exit(1);
        }
        if (dup2(childToParent[1], STDOUT_FILENO) == -1) {
            perror("CGI dup2 childToParent[1] failed");
            _exit(1);
        }
        if (close(parentToChild[0]) == -1) {
            perror("CGI close parentToChild[0] failed");
            _exit(1);
        }
        if (close(childToParent[1]) == -1) {
            perror("CGI close childToParent[1] failed");
            _exit(1);
        }
        for (size_t i = 0; i < openFds.size(); i++) {
            if (close(openFds[i]) == -1) {
                perror("CGI close openFds failed");
                _exit(1);
            }
        }

        String fileName = scriptPath.substr(scriptDir.size());
        if (!scriptDir.empty()) {
            if (chdir(scriptDir.c_str()) != 0) {
                perror("CGI chdir failed");
                _exit(1);
            }
        }

        VectorString       envStrings = buildEnv(resultRouter);
        VectorString       envStorage(envStrings.begin(), envStrings.end());
        std::vector<char*> envp;
        for (size_t i = 0; i < envStorage.size(); i++)
            envp.push_back(const_cast<char*>(envStorage[i].c_str()));
        envp.push_back(NULL);
        VectorString argvStorage;
        argvStorage.push_back(interpreter);
        argvStorage.push_back(fileName.substr(1));
        std::vector<char*> argv;
        for (size_t i = 0; i < argvStorage.size(); i++)
            argv.push_back(const_cast<char*>(argvStorage[i].c_str()));
        argv.push_back(NULL);

        execve(argv[0], &argv[0], &envp[0]);
        _exit(1);
    }

    close(parentToChild[0]);
    close(childToParent[1]);
    setNonBlocking(parentToChild[1]);
    setNonBlocking(childToParent[0]);
    _cgi->init(pid, parentToChild[1], childToParent[0], resultRouter.getRequest().getBody());
    return true;
}
// ─── Static helpers ──────────────────────────────────────────────────────────

bool CgiHandler::parseOutput(const String& raw, HttpResponse& response) {
    size_t headerEnd    = raw.find("\r\n\r\n");
    size_t headerEndLen = 4;
    if (headerEnd == String::npos) {
        headerEnd    = raw.find("\n\n");
        headerEndLen = 2;
    }
    if (headerEnd == String::npos)
        return false;

    String            headerPart = raw.substr(0, headerEnd);
    String            bodyPart   = raw.substr(headerEnd + headerEndLen);
    std::stringstream ss(headerPart);
    String            line;
    bool              statusSet = false;

    while (std::getline(ss, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        size_t colon = line.find(':');
        if (colon == String::npos)
            continue;
        String key = trimSpaces(line.substr(0, colon));
        String val = trimSpaces(line.substr(colon + 1));
        if (toLowerWords(key) == "status") {
            int    code  = atoi(val.c_str());
            String msg   = "OK";
            size_t space = val.find(' ');
            if (space != String::npos)
                msg = val.substr(space + 1);
            response.setStatus(code, msg);
            statusSet = true;
        } else if (toLowerWords(key) == "set-cookie") {
            response.addSetCookie(val);
        } else {
            response.addHeader(key, val);
        }
    }
    if (!statusSet)
        response.setStatus(HTTP_OK, "OK");
    response.addHeader("Server", "Webserv/1.0");
    response.setBody(bodyPart);
    return true;
}
VectorString CgiHandler::buildEnv(const RouteResult& resultRouter) const {
    VectorString env;

    const HttpRequest&    req = resultRouter.getRequest();
    const LocationConfig* loc = resultRouter.getLocation();
    if (!loc)
        return env;
    // --- Server info ---
    env.push_back("GATEWAY_INTERFACE=" + String(CGI_INTERFACE)); // CGI/1.1
    env.push_back("SERVER_NAME=" + resultRouter.getServer()->getServerName());
    env.push_back("SERVER_SOFTWARE=Webserv/1.0");
    env.push_back("SERVER_PORT=" + typeToString<int>(req.getPort()));
    env.push_back("SERVER_PROTOCOL=" + req.getHttpVersion());
    env.push_back("REQUEST_METHOD=" + req.getMethod());
    // --- Request info ---
    // Calculate SCRIPT_NAME (URI part before PATH_INFO)
    String uri        = req.getUri();
    String pathInfo   = resultRouter.getRemainingPath();
    String scriptName = uri.substr(0, uri.size() - pathInfo.size());
    env.push_back("REQUEST_URI=" + uri);
    env.push_back("QUERY_STRING=" + req.getQueryString());
    env.push_back("SCRIPT_NAME=" + scriptName);
    env.push_back("SCRIPT_FILENAME=" + resultRouter.getPathRootUri());
    env.push_back("PATH_INFO=" + uri);
    if (!pathInfo.empty()) {
        env.push_back("PATH_TRANSLATED=" + joinPaths(loc->getRoot(), pathInfo));
    } else {
        // Fallback for cgi_tester: if pathInfo is empty, some versions expect URI in PATH_INFO
        env.push_back("PATH_TRANSLATED=" + resultRouter.getPathRootUri());
    }
    env.push_back("REDIRECT_STATUS=200");
    env.push_back("REMOTE_ADDR=" + resultRouter.getRemoteAddress());
    // --- POST info ---
    if (!req.getContentType().empty())
        env.push_back("CONTENT_TYPE=" + req.getContentType());
    else
        env.push_back("CONTENT_TYPE=");
    env.push_back("CONTENT_LENGTH=" + typeToString<size_t>(req.getContentLength()));
    env.push_back("REMOTE_HOST=" + req.getHost());

    // --- HTTP headers ---
    const MapString& headers = req.getHeaders();
    for (MapString::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        String key = it->first;
        String val = it->second;

        // sanitize header name
        for (size_t i = 0; i < key.size(); i++) {
            if (key[i] == '-')
                key[i] = '_';
            else if (!std::isalnum(key[i]) && key[i] != '_')
                key[i] = '_';
        }

        // sanitize header value
        String sanitizedVal;
        for (size_t i = 0; i < val.size(); i++) {
            if (val[i] >= 0x20 && val[i] <= 0x7E)
                sanitizedVal += val[i];
        }

        env.push_back("HTTP_" + toUpperWords(key) + "=" + sanitizedVal);
    }

    return env;
}
