#include "HttpRequest.hpp"
/* Header Section */
// POST ?user=loay&id=42 HTTP/1.1\r\n (request line)
// Host: localhost:8080\r\n (header lines)
// User-Agent: curl/7.88.1\r\n (header lines)
// Accept: */*\r\n (header lines)
// Content-Type: text/plain\r\n (header lines)
// Content-Length: 11\r\n (header lines)
// Connection: keep-alive\r\n\r\n (\r\n\r\n indicates end of headers)

/* Body Section */
// hello world

HttpRequest::HttpRequest()
    : method(""),
      uri(""),
      httpVersion(""),
      queryString(""),
      fragment(""),
      headers(),
      body(""),
      contentType(""),
      contentLength(0),
      host(""),
      port(-1) {}

HttpRequest::HttpRequest(const HttpRequest &other)
    : method(other.method),
      uri(other.uri),
      httpVersion(other.httpVersion),
      queryString(other.queryString),
      fragment(other.fragment),
      headers(other.headers),
      body(other.body),
      contentType(other.contentType),
      contentLength(other.contentLength),
      host(other.host),
      port(other.port) {}

HttpRequest &HttpRequest::operator=(const HttpRequest &other) {
    if (this != &other) {
        method       = other.method;
        uri          = other.uri;
        httpVersion  = other.httpVersion;
        queryString  = other.queryString;
        fragment     = other.fragment;
        headers      = other.headers;
        body         = other.body;
        contentType  = other.contentType;
        contentLength= other.contentLength;
        host         = other.host;
        port         = other.port;
    }
    return *this;
}

HttpRequest::~HttpRequest() {
    headers.clear();
}

bool HttpRequest::parse(const std::string& raw) {
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;
    std::string headerSection = raw.substr(0, headerEnd);
    std::string bodySection   = raw.substr(headerEnd + 4); // +4 to skip \r\n\r\n
    if (!parseHeaders(headerSection))
        return Logger::error("Failed to parse headers");
    if (!parseBody(bodySection))
        return Logger::error("Failed to parse body");
    return true;
}
bool HttpRequest::parseHeaders(const std::string& headerSection) {
    size_t lineEnd = headerSection.find("\r\n");
    if (lineEnd == std::string::npos)
        return Logger::error("Failed to find end of request line");

    std::string              requestLine = headerSection.substr(0, lineEnd);
    std::vector<std::string> values;
    if (!parseKeyValue(requestLine, method, values))
        return Logger::error("Failed to parse request line");
    if (values.size() != 2)
        return Logger::error("Invalid request line format");
    uri         = values[0];
    httpVersion = values[1];
    if (method.empty() || uri.empty() || httpVersion.empty())
        return Logger::error("Empty method, URI, or HTTP version");
    method = toUpperWords(method);
    if (!checkAllowedMethods(method))
        return Logger::error("Method not allowed");
    if (!splitByChar(uri, uri, fragment, '#'))
        fragment = "";
    if (!splitByChar(uri, uri, queryString, '?'))
        queryString = "";
    size_t pos = lineEnd + 2; // +2 to skip \r\n
    while (pos < headerSection.size()) {
        lineEnd = headerSection.find("\r\n", pos);

        // If no more \r\n found, process remaining content as last header
        if (lineEnd == std::string::npos) {
            lineEnd = headerSection.size();
        }

        std::string line = headerSection.substr(pos, lineEnd - pos);
        if (line.empty())
            break;

        std::string key, value;
        if (!splitByChar(line, key, value, ':'))
            return Logger::error("Failed to parse header line");

        std::string headerKey = toLowerWords(trimSpacesComments(key));
        std::string headerVal = trimSpacesComments(value);

        headers[headerKey] = headerVal;
        pos                = lineEnd + 2;
    }
    if (!headers["content-type"].empty())
        contentType = headers["content-type"];
    if (!headers["content-length"].empty()) {
        std::stringstream ss(headers["content-length"]);
        ss >> contentLength;
    } else {
        contentLength = 0;
    }
    std::string hostKey = headers["host"];
    size_t      sepHost = hostKey.find(':');
    if (sepHost == std::string::npos)
        return Logger::error("Missing port in Host header");
    host = hostKey.substr(0, sepHost);
    port = stringToType<int>(hostKey.substr(sepHost + 1));
    return true;
}

bool HttpRequest::parseBody(const std::string& bodySection) {
    body = bodySection;
    if (!headers["content-length"].empty()) {
        if (body.size() != contentLength)
            return Logger::error("Body length does not match Content-Length");
    } else {
        if (!body.empty())
            return Logger::error("Body present without Content-Length header");
    }
    return true;
}

// Getters
std::string HttpRequest::getMethod() const {
    return method;
}
std::string HttpRequest::getUri() const {
    return uri;
}
std::string HttpRequest::getHeader(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end()) {
        return it->second;
    }
    return "";
}
std::string HttpRequest::getBody() const {
    return body;
}
size_t HttpRequest::getContentLength() const {
    return contentLength;
}
std::string HttpRequest::getContentType() const {
    return contentType;
}

std::string HttpRequest::getHost() const {
    return host;
}
int HttpRequest::getPort() const {
    return port;
}
// Validators
bool HttpRequest::isComplete() const {
    return !method.empty() && !uri.empty() && !httpVersion.empty();
}
bool HttpRequest::hasBody() const {
    return !body.empty();
}