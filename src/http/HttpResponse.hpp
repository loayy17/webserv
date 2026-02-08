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
    void   setBody(const String&);
    String getBody() const;
    String toString() const;

   private:
    int       statusCode;
    String    statusMessage;
    MapString headers;
    String    body;
};

#endif
