#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <iostream>
#include <map>
#include <sstream>
#include "../utils/Utils.hpp"
#include "../utils/Types.hpp"

class HttpRequest {
   private:
    std::string    method;        // GET, POST, DELETE
    std::string    uri;           // /path/to/resource
    std::string    httpVersion;   // HTTP/1.1
    std::string    queryString;   // ?key=value
    std::string    fragment;      // #section
    MapString      headers;       // Header key-value pairs
    std::string    body;          // Request body
    std::string    contentType;   // e.g., text/html Mime type
    size_t         contentLength; // e.g., 348
    std::string    host;          // Host from Host header
    int            port;          // Port from Host header
    MapString      cookies;       // Cookies from Cookie header
    int            errorCode;     // HTTP error code (0 if no error)

   public:
    HttpRequest();
    HttpRequest(const HttpRequest& other);
    HttpRequest& operator=(const HttpRequest& other);
    ~HttpRequest();
    // Parsing
    bool parse(const std::string& raw);
    bool parseHeaders(const std::string& headerSection);
    bool parseBody(const std::string& bodySection);
    void parseCookies(const std::string& cookieHeader);

    // Getters
    std::string                               getMethod() const;
    std::string                               getUri() const;
    std::string                               getHttpVersion() const;
    std::string                               getHeader(const std::string& key) const;
    const MapString& getHeaders() const;
    std::string                               getBody() const;
    size_t                                    getContentLength() const;
    std::string                               getContentType() const;
    std::string                               getHost() const;
    int                                       getPort() const;
    std::string                               getCookie(const std::string& key) const;
    const MapString& getCookies() const;
    int                                       getErrorCode() const;

    // Validators
    bool isComplete() const;
    bool hasBody() const;
    bool validateHostHeader();
    bool validateContentLength();
};

#endif