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

    String extension = extractFileExtension(resultRouter.getPathRootUri());
    if (extension.empty())
        return Logger::error("Failed to parse CGI extension");

    String interpreter = loc->getCgiInterpreter(extension);
    int    parentToChild[2];
    int    childToParent[2];
    if (pipe(parentToChild) == -1)
        return Logger::error("Failed to create pipe for CGI");
    if (pipe(childToParent) == -1) {
        close(parentToChild[0]);
        close(parentToChild[1]);
        return Logger::error("Failed to create pipe for CGI");
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(parentToChild[0]);
        close(parentToChild[1]);
        close(childToParent[0]);
        close(childToParent[1]);
        return Logger::error("Failed to fork CGI process");
    }

    if (pid == 0) {
        close(parentToChild[1]);
        close(childToParent[0]);
        dup2(parentToChild[0], STDIN_FILENO);
        dup2(childToParent[1], STDOUT_FILENO);
        close(parentToChild[0]);
        close(childToParent[1]);
        for (size_t i = 0; i < openFds.size(); ++i) 
                close(openFds[i]);

        String scriptDir = extractDirectoryFromPath(resultRouter.getPathRootUri());
        if (!scriptDir.empty() && chdir(scriptDir.c_str()) != 0) {
            perror("chdir failed");
            _exit(1);
        }

        VectorString       envStrings = buildEnv(resultRouter);
        std::vector<char*> env        = convertEnv(envStrings);

        char* argv[3];
        argv[0] = const_cast<char*>(interpreter.c_str());
        argv[1] = const_cast<char*>(resultRouter.getPathRootUri().c_str());
        argv[2] = NULL;

        execve(argv[0], argv, &env[0]);
        perror("execve failed");
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
            response.setStatus(atoi(val.c_str()), "OK");
            statusSet = true;
        } else {
            response.addHeader(key, val);
        }
    }
    if (!statusSet)
        response.setStatus(HTTP_OK, "OK");
    response.setBody(bodyPart);
    return true;
}

// ─── Private helpers ─────────────────────────────────────────────────────────

VectorString CgiHandler::buildEnv(const RouteResult& resultRouter) const {
    VectorString env;

    const HttpRequest&    req = resultRouter.getRequest();
    const LocationConfig* loc = resultRouter.getLocation();
    if (!loc)
        return env;

    env.push_back("REQUEST_METHOD=" + req.getMethod());
    env.push_back("QUERY_STRING=" + req.getQueryString());
    env.push_back("CONTENT_LENGTH=" + typeToString<size_t>(req.getContentLength()));
    if (!req.getContentType().empty())
        env.push_back("CONTENT_TYPE=" + req.getContentType());
    env.push_back("SCRIPT_NAME=" + loc->getPath());
    env.push_back("SCRIPT_FILENAME=" + resultRouter.getPathRootUri());
    env.push_back("PATH_INFO=" + resultRouter.getRemainingPath());
    env.push_back("SERVER_NAME=" + req.getHost());
    env.push_back("SERVER_PORT=" + typeToString<int>(req.getPort()));
    env.push_back("GATEWAY_INTERFACE=" + String(CGI_INTERFACE));
    env.push_back("SERVER_PROTOCOL=" + String(SERVER_PROTOCOL));

    const MapString& headers = req.getHeaders();
    for (MapString::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        String key = it->first;
        for (size_t i = 0; i < key.size(); ++i)
            if (key[i] == '-')
                key[i] = '_';
        env.push_back("HTTP_" + toUpperWords(key) + "=" + it->second);
    }
    return env;
}

std::vector<char*> CgiHandler::convertEnv(const VectorString& env) const {
    std::vector<char*> result;
    for (size_t i = 0; i < env.size(); ++i)
        result.push_back(const_cast<char*>(env[i].c_str()));
    result.push_back(NULL);
    return result;
}
