#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP
#include <iostream>
#include <map>
#include "../utils/Utils.hpp"

class HttpResponse {
   private:
    int                                statusCode;
    std::string                        statusMessage;
    std::map<std::string, std::string> headers;
    std::string                        body;

   public:
    HttpResponse();
    HttpResponse(const HttpResponse& other);
    HttpResponse& operator=(const HttpResponse& other);
    ~HttpResponse();

    void        setStatus(int code, const std::string& message);
    void        addHeader(const std::string& key, const std::string& value);
    void        setBody(const std::string& content);
    std::string httpToString() const;
};

#endif