#ifndef I_HANDLER_HPP
#define I_HANDLER_HPP
#include "../http/HttpResponse.hpp"
#include "../http/RouteResult.hpp"

class IHandler {
   public:
    virtual ~IHandler() {}
    virtual bool handle(const RouteResult& resultRouter, HttpResponse& response) const = 0;
};

#endif