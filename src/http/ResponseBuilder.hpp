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
#include "Router.hpp"

class ResponseBuilder {
   public:
    ResponseBuilder();
    ResponseBuilder(const MimeTypes& _mimeTypes);
    ~ResponseBuilder();

    HttpResponse build(const Router& router);
    HttpResponse buildError(int code, const std::string& msg);

   private:
    MimeTypes mimeTypes;

    bool handleStatic(HttpResponse& response, const Router& router) const;
    bool handleDirectory(HttpResponse& response, const Router& router) const;
    bool handleCgi(HttpResponse& response, const Router& router) const;
    bool handleUpload(HttpResponse& response, const Router& router) const;
    void handleError(HttpResponse& response, const Router& router);

    // Helpers
    String getErrorPagePath(const Router& router, int code) const;
};

#endif
