#ifndef RESPONSE_BUILDER_HPP
#define RESPONSE_BUILDER_HPP

#include <fstream>
#include "../config/MimeTypes.hpp"
#include "../handlers/CgiHandler.hpp"
#include "../handlers/DirectoryListingHandler.hpp"
#include "../handlers/ErrorPageHandler.hpp"
#include "../handlers/StaticFileHandler.hpp"
#include "../handlers/UploaderHandler.hpp"
#include "../utils/Utils.hpp"
#include "HttpResponse.hpp"
#include "RouteResult.hpp"

class ResponseBuilder {
   public:
    ResponseBuilder();
    ResponseBuilder(const MimeTypes& _mimeTypes);
    ~ResponseBuilder();

    HttpResponse build(const RouteResult& resultRouter);
    HttpResponse buildError(int code, const std::string& msg);

   private:
    MimeTypes mimeTypes;

    bool handleStatic(HttpResponse& response, const RouteResult& resultRouter) const;
    bool handleDirectory(HttpResponse& response, const RouteResult& resultRouter) const;
    bool handleCgi(HttpResponse& response, const RouteResult& resultRouter) const;
    bool handleUpload(HttpResponse& response, const RouteResult& resultRouter) const;
    void handleError(HttpResponse& response, const RouteResult& resultRouter);

    // Helpers
    String getErrorPagePath(const RouteResult& resultRouter, int code) const;
};

#endif
