#ifndef I_HANDLER_HPP
#define I_HANDLER_HPP
#include "../http/HttpResponse.hpp"
#include "../http/Router.hpp"

class IHandler {
   public:
    virtual ~IHandler() {}
    virtual bool handle(const Router& router, HttpResponse& response) const = 0;
};

#endif