#ifndef RESPONSE_BUILDER_HPP
#define RESPONSE_BUILDER_HPP

#include <fstream>
#include "../config/MimeTypes.hpp"
#include "../handlers/CgiHandler.hpp"
#include "../handlers/CgiProcess.hpp"
#include "../handlers/DeleteHandler.hpp"
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
    ResponseBuilder(const ResponseBuilder& other);
    ResponseBuilder& operator=(const ResponseBuilder& other);
    ~ResponseBuilder();

    HttpResponse build(const RouteResult& resultRouter, CgiProcess* cgi = NULL, const VectorInt& openFds = VectorInt());
    HttpResponse buildError(int code, const std::string& msg);
    HttpResponse buildCgiResponse(CgiProcess& cgi);

   private:
    MimeTypes mimeTypes;

    bool handleStatic(HttpResponse& response, const RouteResult& resultRouter) const;
    bool handleDelete(HttpResponse& response, const RouteResult& resultRouter) const;
    bool handleDirectory(HttpResponse& response, const RouteResult& resultRouter) const;
    bool handleUpload(HttpResponse& response, const RouteResult& resultRouter) const;
    bool handleCgi(HttpResponse& response, const RouteResult& resultRouter, CgiProcess* cgi, const VectorInt& openFds) const;
    void handleError(HttpResponse& response, const RouteResult& resultRouter);

    // Helpers
    String getErrorPagePath(const RouteResult& resultRouter, int code) const;
};

#endif
