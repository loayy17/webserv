#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <map>
#include <string>
#include "../utils/Utils.hpp"

class HttpResponse {
   public:
    HttpResponse();
    HttpResponse(const HttpResponse& other);
    HttpResponse& operator=(const HttpResponse& other);
    ~HttpResponse();

    void   setStatus(int code, const String& msg);
    void   addHeader(const String&, const String&);
    void   addSetCookie(const String& cookie);
    void   setResponseHeaders(const String& contentType, size_t contentLength);
    void   setBody(const String&);
    void   setHttpVersion(const String& version);
    String getBody() const;
    String toString();
    int    getStatusCode() const;

    String getStatusMessage() const;

   private:
    int          statusCode;
    String       statusMessage;
    String       httpVersion;
    MapString    headers;
    VectorString setCookies;
    String       body;
};

#endif
