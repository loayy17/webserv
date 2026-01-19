#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>

class HttpRequest {
   private:
    std::string uri;

   public:
    HttpRequest();
    ~HttpRequest();
    
    void        parseRequest(const std::string& rawRequest);
    std::string getUri() const;
};

#endif