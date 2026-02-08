#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <vector>
#include "../http/HttpResponse.hpp"
#include "IHandler.hpp"

class CgiHandler : public IHandler {
   public:
    CgiHandler();
    CgiHandler(const CgiHandler& other);
    CgiHandler& operator=(const CgiHandler& other);
    ~CgiHandler();

    bool handle(const Router& router, HttpResponse& response) const;

   private:
    std::vector<std::string> buildEnv(const Router& router) const;
    std::vector<char*>       convertEnv(const std::vector<std::string>& env) const;
};

#endif
