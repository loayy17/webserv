#include "HttpResponse.hpp"

HttpResponse::HttpResponse() : statusCode(), statusMessage(), httpVersion(HTTP_VERSION_1_1), headers(), setCookies(), body() {}
HttpResponse::HttpResponse(const HttpResponse& other)
    : statusCode(other.statusCode),
      statusMessage(other.statusMessage),
      httpVersion(other.httpVersion),
      headers(other.headers),
      setCookies(other.setCookies),
      body(other.body) {}

HttpResponse& HttpResponse::operator=(const HttpResponse& other) {
    if (this != &other) {
        statusCode    = other.statusCode;
        statusMessage = other.statusMessage;
        httpVersion   = other.httpVersion;
        headers       = other.headers;
        setCookies    = other.setCookies;
        body          = other.body;
    }
    return *this;
}
HttpResponse::~HttpResponse() {}

void HttpResponse::setStatus(int code, const String& msg) {
    statusCode    = code;
    statusMessage = msg;
}
void HttpResponse::addHeader(const String& key, const String& value) {
    headers[key] = value;
}

void HttpResponse::addSetCookie(const String& cookie) {
    setCookies.push_back(cookie);
}

void HttpResponse::setResponseHeaders(const String& contentType, size_t contentLength) {
    addHeader(HEADER_CONTENT_TYPE, contentType);
    addHeader(HEADER_CONTENT_LENGTH, typeToString<size_t>(contentLength));
}

int HttpResponse::getStatusCode() const {
    return statusCode;
}

String HttpResponse::getStatusMessage() const {
    return statusMessage;
}

void HttpResponse::setBody(const String& _body) {
    body = _body;
}

void HttpResponse::setHttpVersion(const String& version) {
    httpVersion = version;
}

String HttpResponse::getBody() const {
    return body;
}

String HttpResponse::toString() {
    String ss;
    ss += httpVersion + " " + typeToString<int>(statusCode) + " " + statusMessage + "\r\n";
    addHeader(HEADER_DATE, formatDateTime());
    addHeader(HEADER_SERVER, "Webserv/1.0");
    for (MapString::const_iterator it = headers.begin(); it != headers.end(); ++it)
        ss += it->first + ": " + it->second + "\r\n";
    for (size_t i = 0; i < setCookies.size(); ++i)
        ss += String(HEADER_SET_COOKIE) + ": " + setCookies[i] + "\r\n";

    ss += "\r\n";
    ss += body;
    return ss;
}
