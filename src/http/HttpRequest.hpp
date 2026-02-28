#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <iostream>
#include <map>
#include <sstream>
#include "../utils/Utils.hpp"

class HttpRequest {
   private:
    String    method;        // GET, POST, DELETE
    String    uri;           // /path/to/resource
    String    httpVersion;   // HTTP/1.1
    String    queryString;   // ?key=value
    String    fragment;      // #section
    MapString headers;       // Header key-value pairs
    String    body;          // Request body
    String    contentType;   // e.g., text/html Mime type
    size_t    contentLength; // e.g., 348
    String    host;          // Host from Host header
    int       port;          // Port from Host header
    MapString cookies;       // Cookies from Cookie header
    int       errorCode;     // HTTP error code (0 if no error)

   public:
    HttpRequest();
    HttpRequest(const HttpRequest& other);
    HttpRequest& operator=(const HttpRequest& other);
    ~HttpRequest();
    void clear();
    // Parsing
    bool parse(const String& raw);
    bool parseHeaders(const String& headerSection);
    bool parseBody(const String& bodySection);
    void parseCookies(const String& cookieHeader);

    // Getters
    const String&    getMethod() const;
    const String&    getUri() const;
    const String&    getHttpVersion() const;
    String           getHeader(const String& key) const;
    const MapString& getHeaders() const;
    const String&    getBody() const;
    size_t           getContentLength() const;
    const String&    getContentType() const;
    const String&    getHost() const;
    int              getPort() const;
    String           getCookie(const String& key) const;
    const MapString& getCookies() const;
    int              getErrorCode() const;
    const String&    getQueryString() const;

    // Setters
    void setPort(int serverPort);

    // Validators
    bool isComplete() const;
    bool hasBody() const;
    bool validateHostHeader();
    bool validateContentLength();
};

#endif