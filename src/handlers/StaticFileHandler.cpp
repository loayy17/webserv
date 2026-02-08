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

bool StaticFileHandler::handle(const Router& router, HttpResponse& response) const {
    // TODO
    String path = router.getPathRootUri();
    String content;
    if (!readFileContent(path, content))
        return false;
    response.setStatus(200, "OK");
    response.addHeader("Content-Type", mimeTypes.get(path));
    response.addHeader("Content-Length", typeToString<size_t>(content.size()));
    response.setBody(content);
    return true;
}
