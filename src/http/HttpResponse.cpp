#include "HttpResponse.hpp"

HttpResponse::HttpResponse(const HttpResponse& other)
    : statusCode(other.statusCode),
      statusMessage(other.statusMessage),
      headers(other.headers),
      body(other.body) {}

HttpResponse& HttpResponse::operator=(const HttpResponse& other) {
    if (this != &other) {
        statusCode    = other.statusCode;
        statusMessage = other.statusMessage;
        headers       = other.headers;
        body          = other.body;
    }
    return *this;
}

HttpResponse::HttpResponse() : statusCode(200), statusMessage("OK") {}

HttpResponse::~HttpResponse() {
    headers.clear();
}

void HttpResponse::setStatus(int code, const std::string& message) {
    statusCode    = code;
    statusMessage = message;
}

void HttpResponse::addHeader(const std::string& key, const std::string& value) {
    headers[key] = value;
}

void HttpResponse::setBody(const std::string& content) {
    body = content;
    addHeader("Content-Length", typeToString(content.length()));
}

std::string HttpResponse::httpToString() const {
    std::string response = "HTTP/1.1 ";
    response += typeToString(statusCode);
    response += " " + statusMessage + "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        response += it->first + ": " + it->second + "\r\n";
    }

    response += "\r\n" + body;
    return response;
}