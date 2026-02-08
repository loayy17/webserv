#include "HttpResponse.hpp"

HttpResponse::HttpResponse() : statusCode(), statusMessage(), headers(), body() {}
HttpResponse::HttpResponse(const HttpResponse& other)
    : statusCode(other.statusCode), statusMessage(other.statusMessage), headers(other.headers), body(other.body) {}
HttpResponse& HttpResponse::operator=(const HttpResponse& other) {
    if (this != &other) {
        statusCode    = other.statusCode;
        statusMessage = other.statusMessage;
        headers       = other.headers;
        body          = other.body;
    }
    return *this;
}
HttpResponse::~HttpResponse() {
    headers.clear();
}

void HttpResponse::setStatus(int code, const String& msg) {
    statusCode    = code;
    statusMessage = msg;
}
void HttpResponse::addHeader(const String&, const String&) {
    String key, value;
    String valueFind = getValue(headers, key, String());
    headers[key]     = valueFind.empty() ? value : valueFind + ", " + value;
}
void HttpResponse::setBody(const String& _body) {
    body = _body;
}

String HttpResponse::getBody() const {
    return body;
}

String HttpResponse::toString() const {
    String ss;
    ss += "HTTP/1.1 " + typeToString<int>(statusCode) + " " + statusMessage + "\r\n";

    for (MapString::const_iterator it = headers.begin(); it != headers.end(); ++it)
        ss += it->first + ": " + it->second + "\r\n";

    ss += "\r\n";
    ss += body;

    return ss;
}
