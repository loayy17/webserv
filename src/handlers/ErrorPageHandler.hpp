#ifndef ERROR_PAGE_HANDLER_HPP
#define ERROR_PAGE_HANDLER_HPP

#include "../config/MimeTypes.hpp"
#include "../http/HttpResponse.hpp"
#include "../http/RouteResult.hpp"
#include "../utils/Utils.hpp"

class ErrorPageHandler {
   public:
    ErrorPageHandler() {}
    ~ErrorPageHandler() {}

    // Generates HTML string for error
    std::string generateHtml(int code, const std::string& msg) const;

    // Builds HttpResponse based on Router (uses config error_page if exists)
    void handle(HttpResponse& response, const RouteResult& resultRouter, const MimeTypes& mimeTypes) const;
};

#endif
