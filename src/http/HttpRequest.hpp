#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <iostream>
#include <map>
#include <sstream>
#include "../utils/Utils.hpp"

class HttpRequest {
   private:
    std::string                        method;        // GET, POST, DELETE
    std::string                        uri;           // /path/to/resource
    std::string                        httpVersion;   // HTTP/1.1
    std::string                        queryString;   // ?key=value
    std::string                        fragment;      // #section
    std::map<std::string, std::string> headers;       // Header key-value pairs
    std::string                        body;          // Request body
    std::string                        contentType;   // e.g., text/html Mime type
    size_t                             contentLength; // e.g., 348
    std::string                        host;          // Host from Host header
    int                                port;          // Port from Host header

   public:
    HttpRequest();
    HttpRequest(const HttpRequest &other);
    HttpRequest &operator=(const HttpRequest &other);
    ~HttpRequest();
    // Parsing
    bool parse(const std::string& raw);
    bool parseHeaders(const std::string& headerSection);
    bool parseBody(const std::string& bodySection);

    // Getters
    std::string getMethod() const;
    std::string getUri() const;
    std::string getHeader(const std::string& key) const;
    std::string getBody() const;
    size_t      getContentLength() const;
    std::string getContentType() const;
    std::string getHost() const;
    int         getPort() const;

    // Validators
    bool isComplete() const;
    bool hasBody() const;
};

#endif