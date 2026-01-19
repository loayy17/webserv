#include "HttpRequest.hpp"

HttpRequest::HttpRequest() {}
HttpRequest::~HttpRequest() {}

void HttpRequest::parseRequest(const std::string& rawRequest) {
    size_t lineEnd = rawRequest.find("\r\n");
    if (lineEnd == std::string::npos) return;

    std::string requestLine = rawRequest.substr(0, lineEnd);
    size_t methodEnd = requestLine.find(' ');
    if (methodEnd == std::string::npos) return;

    size_t uriEnd = requestLine.find(' ', methodEnd + 1);
    if (uriEnd == std::string::npos) return;
    
    uri = requestLine.substr(methodEnd + 1, uriEnd - methodEnd - 1);
}

std::string HttpRequest::getUri() const {
    return uri;
}