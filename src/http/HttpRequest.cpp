#include "HttpRequest.hpp"

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

HttpRequest::~HttpRequest() {}

bool HttpRequest::parse(const String& raw) {
    errorCode        = 0;
    size_t headerEnd = raw.find(DOUBLE_CRLF);
    if (headerEnd == String::npos) {
        errorCode = HTTP_BAD_REQUEST;
        return false;
    }
    String headerSection = raw.substr(0, headerEnd);
    String bodySection   = raw.substr(headerEnd + 4); // +4 to skip \r\n\r\n
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
bool HttpRequest::parseHeaders(const String& headerSection) {
    size_t lineEnd = headerSection.find(CRLF);
    if (lineEnd == String::npos && (errorCode = HTTP_BAD_REQUEST)) {
        return Logger::error("Failed to find end of request line");
    }

    String       requestLine = headerSection.substr(0, lineEnd);
    VectorString values;
    if (!parseKeyValue(requestLine, method, values) && (errorCode = HTTP_BAD_REQUEST)) {
        return Logger::error("Failed to parse request line");
    }
    if (values.size() != 2 && (errorCode = HTTP_BAD_REQUEST)) {
        return Logger::error("Invalid request line format");
    }

    uri         = values[0];
    httpVersion = values[1];
    if (method.empty() || uri.empty() || httpVersion.empty()) {
        errorCode = HTTP_BAD_REQUEST;
        return Logger::error("Empty method, URI, or HTTP version");
    }
    // ! Validate HTTP version (505 HTTP Version Not Supported)
    if (httpVersion != HTTP_VERSION_1_1 && httpVersion != HTTP_VERSION_1_0) {
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

    if (!splitByChar(uri, uri, fragment, HASH))
        fragment = "";
    if (!splitByChar(uri, uri, queryString, QUESTION))
        queryString = "";
    uri = urlDecode(uri);

    size_t pos = lineEnd + 2; // +2 to skip \r\n
    while (pos < headerSection.size()) {
        lineEnd = headerSection.find(CRLF, pos);

        // If no more \r\n found, process remaining content as last header
        if (lineEnd == String::npos)
            lineEnd = headerSection.size();

        String line = headerSection.substr(pos, lineEnd - pos);
        if (line.empty())
            break;

        String key, value;
        if (!splitByChar(line, key, value, COLON) && (errorCode = HTTP_BAD_REQUEST))
            return Logger::error("Failed to parse header line");

        String headerKey = toLowerWords(trimSpaces(key));
        String headerVal = trimSpaces(value);

        if (headerKey == "content-length") {
            if (hasNonEmptyValue(headers, headerKey)) {
                errorCode = HTTP_BAD_REQUEST;
                return Logger::error("Multiple Content-Length headers not allowed");
            }
            headers[headerKey] = headerVal;
        } else {
            // RFC 7230: Multiple headers with same name should append with comma
            if (!hasNonEmptyValue(headers, headerKey))
                headers[headerKey] = headerVal;
            else
                headers[headerKey] += "," + headerVal;
        }
        pos = lineEnd + 2;
    }

    // ! Validate Host header (required in HTTP/1.1)
    if (!validateHostHeader()) {
        errorCode = HTTP_BAD_REQUEST;
        return Logger::error("Missing or invalid Host header");
    }

    // ! Parse cookies if present
    String cookieHeader = getValue(headers, String(HEADER_COOKIE), String());
    if (!cookieHeader.empty())
        parseCookies(cookieHeader);

    // ! Extract Content-Type
    String ct = getValue(headers, String("content-type"), String());
    if (!ct.empty())
        contentType = ct;

    // ! Validate and extract Content-Length
    if (!validateContentLength()) {
        return Logger::error("Invalid Content-Length header");
    }

    // ! Extract host and port
    String portStr;
    String hostHeader = getValue(headers, String(HEADER_HOST), String());
    if (!hostHeader.empty()) {
        if (!splitByChar(hostHeader, host, portStr, COLON)) {
            host = hostHeader;
        } else if (!stringToType<int>(portStr, port) || port < 1 || port > 65535) {
            errorCode = HTTP_BAD_REQUEST;
            return Logger::error("Invalid port number in Host header");
        }
    }
    return true;
}

bool HttpRequest::parseBody(const String& bodySection) {
    body = bodySection;
    // ! If method typically has a body (POST, PUT, PATCH)
    bool methodExpectsBody = (method == METHOD_POST || method == METHOD_PUT || method == METHOD_PATCH);

    if (hasNonEmptyValue(headers, String("content-length"))) {
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
    if (httpVersion == HTTP_VERSION_1_1) {
        MapString::const_iterator it = headers.find(HEADER_HOST);
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
    const String& clValue = it->second;
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
void HttpRequest::parseCookies(const String& cookieHeader) {
    VectorString cookiePairs;
    splitByString(cookieHeader, cookiePairs, ";");
    for (size_t i = 0; i < cookiePairs.size(); ++i) {
        String key, value;
        if (splitByChar(trimSpaces(cookiePairs[i]), key, value, EQUALS)) {
            cookies[trimSpaces(key)] = trimSpaces(value);
        }
    }
}
// Getters
const String& HttpRequest::getMethod() const {
    return method;
}
const String& HttpRequest::getUri() const {
    return uri;
}
const String& HttpRequest::getHttpVersion() const {
    return httpVersion;
}
String HttpRequest::getHeader(const String& key) const {
    String                    lowerKey = toLowerWords(key);
    MapString::const_iterator it       = headers.find(lowerKey);
    if (it != headers.end()) {
        return it->second;
    }
    return "";
}
const MapString& HttpRequest::getHeaders() const {
    return headers;
}
const String& HttpRequest::getBody() const {
    return body;
}
size_t HttpRequest::getContentLength() const {
    return contentLength;
}
const String& HttpRequest::getContentType() const {
    return contentType;
}

const String& HttpRequest::getHost() const {
    return host;
}
int HttpRequest::getPort() const {
    return port;
}

void HttpRequest::setPort(int serverPort) {
    port = serverPort;
}

// Validators
bool HttpRequest::isComplete() const {
    return !method.empty() && !uri.empty() && !httpVersion.empty();
}
bool HttpRequest::hasBody() const {
    return !body.empty();
}
String HttpRequest::getCookie(const String& key) const {
    const MapString::const_iterator it = cookies.find(key);
    if (it == cookies.end()) {
        return "";
    }
    return it->second;
}
const MapString& HttpRequest::getCookies() const {
    return cookies;
}
int HttpRequest::getErrorCode() const {
    return errorCode;
}

const String& HttpRequest::getQueryString() const {
    return queryString;
}