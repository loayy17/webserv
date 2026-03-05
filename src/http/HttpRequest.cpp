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

void HttpRequest::clear() {
    method      = "";
    uri         = "";
    httpVersion = "";
    queryString = "";
    fragment    = "";
    headers.clear();
    body          = "";
    contentType   = "";
    contentLength = 0;
    host          = "";
    port          = 80;
    cookies.clear();
    errorCode = 0;
}

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
bool HttpRequest::parseRequestLine(const String& requestLine) {
    VectorString values;
    if (!parseKeyValue(requestLine, method, values)) {
        errorCode = HTTP_BAD_REQUEST;
        return Logger::error("Failed to parse request line");
    }
    if (values.size() != 2) {
        errorCode = HTTP_BAD_REQUEST;
        return Logger::error("Invalid request line format");
    }

    uri         = values[0];
    httpVersion = values[1];
    if (method.empty() || uri.empty() || httpVersion.empty()) {
        errorCode = HTTP_BAD_REQUEST;
        return Logger::error("Empty method, URI, or HTTP version");
    }
    if (httpVersion != HTTP_VERSION_1_1 && httpVersion != HTTP_VERSION_1_0) {
        errorCode = HTTP_VERSION_NOT_SUPPORTED;
        return Logger::error("Unsupported HTTP version");
    }
    if (uri.length() > MAX_URI_LENGTH) {
        errorCode = HTTP_URI_TOO_LONG;
        return Logger::error("URI too long");
    }

    method = toUpperWords(method);
    if (!isValidHttpMethod(method)) {
        errorCode = HTTP_NOT_IMPLEMENTED;
        return Logger::error("Method not implemented");
    }

    if (!splitByChar(uri, uri, fragment, HASH))
        fragment = "";
    if (!splitByChar(uri, uri, queryString, QUESTION))
        queryString = "";
    else
        queryString = urlDecode(queryString);
    uri = urlDecode(uri);
    return true;
}

bool HttpRequest::parseHeaders(const String& headerSection) {
    size_t lineEnd    = headerSection.find(CRLF);
    size_t lineEndLen = 2;
    if (lineEnd == String::npos) {
        lineEnd    = headerSection.find("\n");
        lineEndLen = 1;
    }

    if (lineEnd == String::npos) {
        lineEnd    = headerSection.size();
        lineEndLen = 0;
    }

    if (lineEnd == 0) {
        errorCode = HTTP_BAD_REQUEST;
        return Logger::error("Empty request line");
    }

    if (!parseRequestLine(headerSection.substr(0, lineEnd)))
        return false;

    size_t pos = lineEnd + lineEndLen;
    while (pos < headerSection.size()) {
        lineEnd    = headerSection.find(CRLF, pos);
        lineEndLen = 2;
        if (lineEnd == String::npos) {
            lineEnd    = headerSection.find("\n", pos);
            lineEndLen = 1;
        }

        // If no more line endings found, process remaining content as last header
        if (lineEnd == String::npos) {
            lineEnd    = headerSection.size();
            lineEndLen = 0;
        }

        String line = headerSection.substr(pos, lineEnd - pos);
        if (line.empty())
            break;

        String key, value;
        if (!splitByChar(line, key, value, COLON)) {
            errorCode = HTTP_BAD_REQUEST;
            return Logger::error("Failed to parse header line");
        }

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
        pos = lineEnd + lineEndLen;
    }

    if (!validateHostHeader()) {
        errorCode = HTTP_BAD_REQUEST;
        return Logger::error("Missing or invalid Host header");
    }

    String cookieHeader = getValue(headers, String(HEADER_COOKIE), String());
    if (!cookieHeader.empty())
        parseCookies(cookieHeader);

    String ct = getValue(headers, String("content-type"), String());
    if (!ct.empty())
        contentType = ct;

    if (!validateContentLength()) {
        return Logger::error("Invalid Content-Length header");
    }

    String portStr;
    String hostHeader = getValue(headers, String(HEADER_HOST), String());
    if (!hostHeader.empty()) {
        if (!splitByChar(hostHeader, host, portStr, COLON)) {
            host = hostHeader;
        } else if (!stringToType<int>(portStr, port) || port < 1 || port > MAX_PORT) {
            errorCode = HTTP_BAD_REQUEST;
            return Logger::error("Invalid port number in Host header");
        }
    }
    return true;
}

bool HttpRequest::parseBody(const String& bodySection) {
    body = bodySection;
    bool methodExpectsBody = isMethodWithBody(method);

    if (hasNonEmptyValue(headers, String("content-length"))) {
        if (body.size() != contentLength) {
            errorCode = HTTP_BAD_REQUEST;
            return Logger::error("Body length does not match Content-Length");
        }
    } else {
        if (!body.empty()) {
            if (methodExpectsBody) {
                errorCode = HTTP_LENGTH_REQUIRED;
                return Logger::error("Content-Length required for request with body");
            } else {
                errorCode = HTTP_BAD_REQUEST;
                return Logger::error("Body present without Content-Length header");
            }
        }
    }
    return true;
}

bool HttpRequest::validateHostHeader() {
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
        if (!std::isdigit(static_cast<unsigned char>(clValue[i]))) {
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