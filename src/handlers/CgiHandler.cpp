#include "CgiHandler.hpp"
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"

CgiHandler::CgiHandler() {}
CgiHandler::CgiHandler(const CgiHandler& other) {
    (void)other;
}
CgiHandler& CgiHandler::operator=(const CgiHandler& other) {
    (void)other;
    return *this;
}
CgiHandler::~CgiHandler() {}

// Build environment variables from Router and HttpRequest
VectorString CgiHandler::buildEnv(const Router& router) const {
    std::vector<String> env;

    const HttpRequest&    req = router.getRequest();
    const LocationConfig* loc = router.getLocation();
    if (!loc)
        return env;

    std::string method = req.getMethod();
    env.push_back("REQUEST_METHOD=" + method);
    env.push_back("QUERY_STRING=" + req.getQueryString());

    size_t contentLength = req.getContentLength();
    env.push_back("CONTENT_LENGTH=" + typeToString<size_t>(contentLength));
    if (!req.getContentType().empty())
        env.push_back("CONTENT_TYPE=" + req.getContentType());
    env.push_back("SCRIPT_NAME=" + loc->getPath());
    env.push_back("SCRIPT_FILENAME=" + router.getPathRootUri());
    env.push_back("PATH_INFO=" + router.getRemainingPath());
    env.push_back("SERVER_NAME=" + req.getHost());
    env.push_back("SERVER_PORT=" + typeToString<int>(req.getPort()));
    env.push_back("GATEWAY_INTERFACE=" + String(CGI_INTERFACE));
    env.push_back("SERVER_PROTOCOL=" + String(SERVER_PROTOCOL));

    // Headers
    const MapString& headers = req.getHeaders();
    for (MapString::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        String key = it->first;
        for (size_t i = 0; i < key.size(); ++i) {
            if (key[i] == '-')
                key[i] = '_';
            else
                key[i] = toupper(key[i]);
        }
        env.push_back("HTTP_" + key + "=" + it->second);
    }

    return env;
}

std::vector<char*> CgiHandler::convertEnv(const std::vector<String>& env) const {
    std::vector<char*> result;
    for (size_t i = 0; i < env.size(); ++i)
        result.push_back(const_cast<char*>(env[i].c_str()));
    result.push_back(NULL);
    return result;
}

bool parseCgiOutput(const std::string& raw, HttpResponse& response) {
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        headerEnd = raw.find("\n\n");

    if (headerEnd == std::string::npos)
        return false;

    std::string headerPart = raw.substr(0, headerEnd);
    std::string bodyPart   = raw.substr(headerEnd + 4);

    std::stringstream ss(headerPart);
    std::string       line;

    while (std::getline(ss, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 1);

        key = trimSpaces(key);
        val = trimSpaces(val);

        if (toLowerWords(key) == "status") {
            int code = atoi(val.c_str());
            response.setStatus(code, "OK");
        } else {
            response.addHeader(key, val);
        }
    }

    response.setBody(bodyPart);
    return true;
}

bool CgiHandler::handle(const Router& router, HttpResponse& response) const {
    const LocationConfig* loc     = router.getLocation();
    const HttpRequest     request = router.getRequest();
    if (!loc || !loc->hasCgi())
        return false;

    String scriptName, extension;
    extension = extractFileExtension(router.getPathRootUri());
    if (extension.empty())
        return Logger::error("Failed to parse CGI extension");
        
    String interpreter = loc->getCgiInterpreter(extension);
    int    parentToChild[2]; // this is use for send server body (post) to child to it can access them
    int    childToParent[2]; // this is use for make server read all output from cgi child
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
        // send request from server to child (mean the unused is write in child)
        //  read output from client to server (mean the unsed is read not use i just fill the output)
        close(parentToChild[1]);
        close(childToParent[0]);
        // i must link the used fds
        dup2(parentToChild[0], STDIN_FILENO);  // in parent the fd[1] have data to write
        dup2(childToParent[1], STDOUT_FILENO); // in parent you can write use fd[1] and read it use fd[0]

        // it is linked now close them
        close(parentToChild[0]);
        close(childToParent[1]);

        // convert all requierd data
        VectorString       envStrings = buildEnv(router);
        std::vector<char*> env        = convertEnv(envStrings);

        char* argv[3];
        argv[0] = const_cast<char*>(interpreter.c_str());
        argv[1] = const_cast<char*>(router.getPathRootUri().c_str());
        argv[2] = NULL;

        execve(argv[0], argv, &env[0]);
        perror("execve failed");
        _exit(1);
    }
    // in parent what is fds unused i dont need read from parent the child read them
    // and the write function to child fd not need the child write them
    close(parentToChild[0]);
    close(childToParent[1]);
    if (request.getContentLength() > 0) {
        std::string body = request.getBody();
        write(parentToChild[1], body.c_str(), body.size());
    }
    close(parentToChild[1]);

    String  cgiOutput;
    char    buffer[1024];
    ssize_t n;

    while ((n = read(childToParent[0], buffer, 1023)) > 0) {
        buffer[n] = '\0';
        cgiOutput += buffer;
    }
    close(childToParent[0]);
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        if (!parseCgiOutput(cgiOutput, response))
            return Logger::error("Invalid CGI output");
        response.addHeader(HEADER_CONTENT_LENGTH, typeToString<size_t>(response.getBody().size()));
        return true;
    }
    return Logger::error("CGI execution failed");
}
