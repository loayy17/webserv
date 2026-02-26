#include "StaticFileHandler.hpp"

StaticFileHandler::StaticFileHandler() : mimeTypes() {}

StaticFileHandler::StaticFileHandler(const MimeTypes& _mimeTypes) : mimeTypes(_mimeTypes) {}

StaticFileHandler::StaticFileHandler(const StaticFileHandler& other) : mimeTypes(other.mimeTypes) {}

StaticFileHandler& StaticFileHandler::operator=(const StaticFileHandler& other) {
    if (this != &other) {
        mimeTypes = other.mimeTypes;
    }
    return *this;
}

StaticFileHandler::~StaticFileHandler() {}

bool StaticFileHandler::handle(const RouteResult& resultRouter, HttpResponse& response) const {
    String path   = resultRouter.getPathRootUri();
    String method = resultRouter.getRequest().getMethod();
    String content;
    if (!readFileContent(path, content))
        return false;
    response.setStatus(HTTP_OK, "OK");
    response.addHeader(HEADER_SERVER, "Webserv/1.0");
    response.setResponseHeaders(mimeTypes.get(path), content.size());
    if (method != "HEAD") {
        response.setBody(content);
    }
    return true;
}
