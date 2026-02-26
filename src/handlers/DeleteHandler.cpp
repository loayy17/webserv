#include "DeleteHandler.hpp"

DeleteHandler::DeleteHandler() {}
DeleteHandler::DeleteHandler(const DeleteHandler&) {}
DeleteHandler& DeleteHandler::operator=(const DeleteHandler&) { return *this; }
DeleteHandler::~DeleteHandler() {}

bool DeleteHandler::handle(const RouteResult& resultRouter, HttpResponse& response) const {
    String path = resultRouter.getPathRootUri();

    if (getFileType(path) != SINGLEFILE)
        return false;

    if (std::remove(path.c_str()) != 0)
        return false;

    response.setStatus(HTTP_OK, "OK");
    response.addHeader(HEADER_CONTENT_TYPE, "application/json");
    String body = "{\"message\":\"File deleted successfully\"}";
    response.addHeader(HEADER_CONTENT_LENGTH, typeToString<size_t>(body.size()));
    response.addHeader(HEADER_SERVER, "Webserv/1.0");
    if (resultRouter.getRequest().getMethod() != "HEAD")
        response.setBody(body);
    return true;
}
