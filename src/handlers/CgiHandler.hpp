#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <sys/types.h>
#include <string>
#include <vector>
#include "CgiProcess.hpp"
#include "IHandler.hpp"

class CgiHandler : public IHandler {
   public:
    CgiHandler();
    CgiHandler(CgiProcess& cgi);
    CgiHandler(const CgiHandler& other);
    CgiHandler& operator=(const CgiHandler& other);
    ~CgiHandler();

    bool handle(const RouteResult& resultRouter, HttpResponse& response) const;
    bool handle(const RouteResult& resultRouter, HttpResponse& response, const VectorInt& openFds) const;

    static bool parseOutput(const String& raw, HttpResponse& response);

   private:
    CgiProcess* _cgi;

    VectorString       buildEnv(const RouteResult& resultRouter) const;
};

#endif
