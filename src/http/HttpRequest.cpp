#include "HttpRequest.hpp"
/* Header Section */
// POST ?user=loay&id=42 HTTP/1.1\r\n (request line)
// Host: localhost:8080\r\n (header lines)
// User-Agent: curl/7.88.1\r\n (header lines)
// Accept: */*\r\n (header lines)
// Content-Type: text/plain\r\n (header lines)
// Content-Length: 11\r\n (header lines)
// Connection: keep-alive\r\n\r\n (\r\n\r\n indicates end of headers)
// Cookie: session=42; theme=dark; lang=en\r\n (header lines)

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
      port(80),
      cookies(),
      errorCode(0) {}

HttpRequest::HttpRequest(const HttpRequest& other)
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
      port(other.port),
      cookies(other.cookies),
      errorCode(other.errorCode) {}

HttpRequest& HttpRequest::operator=(const HttpRequest& other) {
    if (this != &other) {
        method        = other.method;
        uri           = other.uri;
        httpVersion   = other.httpVersion;
        queryString   = other.queryString;
        fragment      = other.fragment;
        headers       = other.headers;
        body          = other.body;
        contentType   = other.contentType;
        contentLength = other.contentLength;
        host          = other.host;
        port          = other.port;
        cookies       = other.cookies;
        errorCode     = other.errorCode;
    }
    return *this;
}

HttpRequest::~HttpRequest() {
    headers.clear();
    cookies.clear();
}

bool HttpRequest::parse(const std::string& raw) {
    errorCode        = 0;
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        errorCode = HTTP_BAD_REQUEST;
        return false;
    }
    std::string headerSection = raw.substr(0, headerEnd);
    std::string bodySection   = raw.substr(headerEnd + 4); // +4 to skip \r\n\r\n
    if (!parseHeaders(headerSection)) {
        if (errorCode == 0)
            errorCode = HTTP_BAD_REQUEST;
        return Logger::error("Failed to parse headers");
    }
    if (!parseBody(bodySection)) {
        if (errorCode == 0)
            errorCode = HTTP_BAD_REQUEST;
        return Logger::error("Failed to parse body");
    }
    return true;
}
bool HttpRequest::parseHeaders(const std::string& headerSection) {
    size_t lineEnd = headerSection.find("\r\n");
    if (lineEnd == std::string::npos && (errorCode = HTTP_BAD_REQUEST))
        return Logger::error("Failed to find end of request line");

    std::string  requestLine = headerSection.substr(0, lineEnd);
    VectorString values;
    if (!parseKeyValue(requestLine, method, values) && (errorCode = HTTP_BAD_REQUEST)) {
        return Logger::error("Failed to parse request line");
    }
    if (values.size() != 2 && (errorCode = HTTP_BAD_REQUEST))
        return Logger::error("Invalid request line format");

    uri         = values[0];
    httpVersion = values[1];
    if (method.empty() || uri.empty() || httpVersion.empty()) {
        errorCode = HTTP_BAD_REQUEST;
        return Logger::error("Empty method, URI, or HTTP version");
    }
    // ! Validate HTTP version (505 HTTP Version Not Supported)
    if (httpVersion != "HTTP/1.1" && httpVersion != "HTTP/1.0") {
        errorCode = HTTP_VERSION_NOT_SUPPORTED;
        return Logger::error("Unsupported HTTP version");
    }
    // ! Validate URI length (414 URI Too Long)
    if (uri.length() > MAX_URI_LENGTH && (errorCode = HTTP_URI_TOO_LONG))
        return Logger::error("URI too long");

    method = toUpperWords(method);
    // ! Check if method is recognized (501 Not Implemented for unknown methods)
    if (!checkAllowedMethods(method)) {
        errorCode = HTTP_NOT_IMPLEMENTED;
        return Logger::error("Method not implemented");
    }

    if (!splitByChar(uri, uri, fragment, '#'))
        fragment = "";
    if (!splitByChar(uri, uri, queryString, '?'))
        queryString = "";

    size_t pos = lineEnd + 2; // +2 to skip \r\n
    while (pos < headerSection.size()) {
        lineEnd = headerSection.find("\r\n", pos);

        // If no more \r\n found, process remaining content as last header
        if (lineEnd == std::string::npos)
            lineEnd = headerSection.size();

        std::string line = headerSection.substr(pos, lineEnd - pos);
        if (line.empty())
            break;

        std::string key, value;
        if (!splitByChar(line, key, value, ':') && (errorCode = HTTP_BAD_REQUEST)) 
            return Logger::error("Failed to parse header line");

        std::string headerKey = toLowerWords(trimSpaces(key));
        std::string headerVal = trimSpaces(value);

        // ! RFC 7230: Multiple headers with same name should append with comma
        if (!headers[headerKey].empty()) 
            headers[headerKey] += "," + headerVal;
         else
            headers[headerKey] = headerVal;
        pos = lineEnd + 2;
    }

    // ! Validate Host header (required in HTTP/1.1)
    if (!validateHostHeader()) {
        errorCode = HTTP_BAD_REQUEST;
        return Logger::error("Missing or invalid Host header");
    }

    // ! Parse cookies if present
    if (headers.find("cookie") != headers.end())
        parseCookies(headers["cookie"]);

    // ! Extract Content-Type
    if (!headers["content-type"].empty())
        contentType = headers["content-type"];

    // ! Validate and extract Content-Length
    if (!validateContentLength()) {
        return Logger::error("Invalid Content-Length header");
    }

    // ! Extract host and port
    std::string portStr;
    if (!splitByChar(headers["host"], host, portStr, ':')) {
        host = headers["host"];
    } else {
        port = stringToType<int>(portStr);
    }

    return true;
}

bool HttpRequest::parseBody(const std::string& bodySection) {
    body = bodySection;
    // ! If method typically has a body (POST, PUT, PATCH)
    bool methodExpectsBody = (method == "POST" || method == "PUT" || method == "PATCH");

    if (!headers["content-length"].empty()) {
        // ! Content-Length is present, validate body size matches
        if (body.size() != contentLength) {
            errorCode = HTTP_BAD_REQUEST;
            return Logger::error("Body length does not match Content-Length");
        }
    } else {
        // ! No Content-Length header
        if (!body.empty()) {
            // ! Body present without Content-Length
            if (methodExpectsBody) {
                // ! For POST/PUT/PATCH, this should be 411 Length Required
                errorCode = HTTP_LENGTH_REQUIRED;
                return Logger::error("Content-Length required for request with body");
            } else {
                // ! For GET/DELETE, body without Content-Length is invalid
                errorCode = HTTP_BAD_REQUEST;
                return Logger::error("Body present without Content-Length header");
            }
        }
    }
    return true;
}

bool HttpRequest::validateHostHeader() {
    // ! HTTP/1.1 requires Host header
    if (httpVersion == "HTTP/1.1") {
        std::map<std::string, std::string>::const_iterator it = headers.find("host");
        if (it == headers.end() || it->second.empty()) {
            return false;
        }
    }
    return true;
}

bool HttpRequest::validateContentLength() {
    MapString::const_iterator it = headers.find("content-length");
    if (it == headers.end() || it->second.empty()) {
        contentLength = 0;
        return true;
    }
    const std::string& clValue = it->second;
    for (size_t i = 0; i < clValue.length(); ++i) {
        if (!std::isdigit(clValue[i])) {
            errorCode = HTTP_BAD_REQUEST;
            return false;
        }
    }
    std::stringstream ss(clValue);
    ss >> contentLength;
    if (ss.fail()) {
        errorCode = HTTP_BAD_REQUEST;
        return false;
    }
    return true;
}
// ? example Cookie: "key1=value1; key2=value2; key3=value3" & "session=42; theme=dark; lang=en"
void HttpRequest::parseCookies(const std::string& cookieHeader) {
    std::vector<std::string> cookiePairs;
    splitByString(cookieHeader, cookiePairs, ";");
    for (size_t i = 0; i < cookiePairs.size(); ++i) {
        std::string key, value;
        if (splitByChar(trimSpacesComments(cookiePairs[i]), key, value, '=')) {
            cookies[toLowerWords(trimSpacesComments(key))] = trimSpacesComments(value);
        }
    }
}
// Getters
std::string HttpRequest::getMethod() const {
    return method;
}
std::string HttpRequest::getUri() const {
    return uri;
}
std::string HttpRequest::getHttpVersion() const {
    return httpVersion;
}
std::string HttpRequest::getHeader(const std::string& key) const {
    std::string    lowerKey = toLowerWords(key);
    MapString::const_iterator it       = headers.find(lowerKey);
    if (it != headers.end()) {
        return it->second;
    }
    return "";
}
const MapString& HttpRequest::getHeaders() const {
    return headers;
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
std::string HttpRequest::getCookie(const std::string& key) const {
    std::string    lowerKey = toLowerWords(key);
    return cookies[lowerKey];
}
const MapString& HttpRequest::getCookies() const {
    return cookies;
}
int HttpRequest::getErrorCode() const {
    return errorCode;
}