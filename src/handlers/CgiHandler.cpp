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
        close(parentToChild[1]);
        close(childToParent[0]);
        if (dup2(parentToChild[0], STDIN_FILENO) == -1)
            _exit(1);
        if (dup2(childToParent[1], STDOUT_FILENO) == -1)
            _exit(1);
        close(parentToChild[0]);
        close(childToParent[1]);
        for (size_t i = 0; i < openFds.size(); i++)
            close(openFds[i]);

        String fileName = scriptPath.substr(scriptDir.size());
        if (!scriptDir.empty()) {
            if (chdir(scriptDir.c_str()) != 0)
                _exit(1);
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
    _cgi->init(pid, parentToChild[1], childToParent[0]);
    return true;
}

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
            int               code = 0;
            if (!stringToType<int>(val, code))
                code = HTTP_OK;
            String msg   = getHttpStatusMessage(code);
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
    String bodyData = raw.substr(headerEnd + headerEndLen);
    response.setBody(bodyData);
    return true;
}
VectorString CgiHandler::buildEnv(const RouteResult& resultRouter) const {
    VectorString env;

    const HttpRequest&    req = resultRouter.getRequest();
    const LocationConfig* loc = resultRouter.getLocation();
    if (!loc)
        return env;

    env.push_back("GATEWAY_INTERFACE=" + String(CGI_INTERFACE));
    env.push_back("SERVER_NAME=" + resultRouter.getServer()->getServerName());
    env.push_back("SERVER_SOFTWARE=Webserv/1.0");
    env.push_back("SERVER_PORT=" + typeToString<int>(req.getPort()));
    env.push_back("SERVER_PROTOCOL=" + req.getHttpVersion());
    env.push_back("REQUEST_METHOD=" + req.getMethod());

    String uri        = req.getUri();
    String pathInfo   = resultRouter.getRemainingPath();
    String scriptName = uri.substr(0, uri.size() - pathInfo.size());
    env.push_back("REQUEST_URI=" + uri);
    env.push_back("QUERY_STRING=" + req.getQueryString());
    env.push_back("SCRIPT_NAME=" + scriptName);
    env.push_back("SCRIPT_FILENAME=" + resultRouter.getPathRootUri());
    env.push_back("PATH_INFO=" + uri);
    if (!pathInfo.empty())
        env.push_back("PATH_TRANSLATED=" + joinPaths(loc->getRoot(), pathInfo));
    else
        env.push_back("PATH_TRANSLATED=" + resultRouter.getPathRootUri());
    env.push_back("DOCUMENT_ROOT=" + loc->getRoot());
    env.push_back("REDIRECT_STATUS=200");
    env.push_back("REMOTE_ADDR=" + resultRouter.getRemoteAddress());

    env.push_back("CONTENT_TYPE=" + req.getContentType());
    env.push_back("CONTENT_LENGTH=" + typeToString<size_t>(req.getContentLength()));
    env.push_back("REMOTE_HOST=" + req.getHost());

    const MapString& headers = req.getHeaders();
    for (MapString::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        String key = it->first;
        for (size_t i = 0; i < key.size(); i++)
            key[i] = std::isalnum(static_cast<unsigned char>(key[i])) ? key[i] : '_';
        String val;
        for (size_t i = 0; i < it->second.size(); i++)
            if (it->second[i] >= 0x20 && it->second[i] <= 0x7E)
                val += it->second[i];
        env.push_back("HTTP_" + toUpperWords(key) + "=" + val);
    }
    return env;
}
