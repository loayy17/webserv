#include "HttpResponse.hpp"
#include "../utils/Utils.hpp"

HttpResponse::HttpResponse() : statusCode(200), statusMessage("OK") {}

HttpResponse::~HttpResponse() {}

void HttpResponse::setStatus(int code, const std::string& message) {
    statusCode    = code;
    statusMessage = message;
}

void HttpResponse::addHeader(const std::string& key, const std::string& value) {
    headers[key] = value;
}

void HttpResponse::setBody(const std::string& content) {
    body = content;
    addHeader("Content-Length", toString(content.length()));
}

std::string HttpResponse::httpToString() const {
    std::string response = "HTTP/1.1 ";
    response += toString(statusCode);
    response += " " + statusMessage + "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        response += it->first + ": " + it->second + "\r\n";
    }

    response += "\r\n" + body;
    return response;
}