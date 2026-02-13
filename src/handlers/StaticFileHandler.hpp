#ifndef STATIC_FILE_HANDLER_HPP
#define STATIC_FILE_HANDLER_HPP
#include "../config/MimeTypes.hpp"
#include "../http/HttpResponse.hpp"
#include "../http/RouteResult.hpp"
#include "../utils/Utils.hpp"
#include "IHandler.hpp"

class StaticFileHandler : public IHandler {
   public:
    StaticFileHandler();
    StaticFileHandler(const StaticFileHandler& other);
    StaticFileHandler& operator=(const StaticFileHandler& other);
    StaticFileHandler(const MimeTypes& mimeTypes);
    ~StaticFileHandler();

    bool handle(const RouteResult& resultRouter, HttpResponse& response) const;

   private:
    MimeTypes mimeTypes;
};

#endif
