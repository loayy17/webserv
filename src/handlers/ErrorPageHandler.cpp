#include "ErrorPageHandler.hpp"
#include <fstream>
#include <sstream>

std::string ErrorPageHandler::generateHtml(int code, const std::string& msg) const {
    std::stringstream ss;
    ss << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
       << "<meta charset=\"UTF-8\">\n<title>Error " << code << "</title>\n"
       << "<style>body { font-family: monospace; background:#f0f0f0; color:#333; }</style>\n"
       << "</head>\n<body>\n"
       << "<h1>Error " << code << "</h1>\n<p>" << msg << "</p>\n"
       << "</body>\n</html>\n";
    return ss.str();
}

void ErrorPageHandler::handle(HttpResponse& response, const Router& router, const MimeTypes& mimeTypes) const {
    int         code = router.getStatusCode() ? router.getStatusCode() : 500;
    std::string msg  = router.getErrorMessage().empty() ? "Error" : router.getErrorMessage();

    std::string body;
    // Check for custom error page in location first, then server
    const std::string customPage = router.getLocation() ? router.getLocation()->getErrorPage(code) : "";
    if (!customPage.empty() && readFileContent(customPage, body)) {
        response.setStatus(code, "Error");
        response.addHeader("Content-Type", mimeTypes.get(customPage));
        response.addHeader("Content-Length", typeToString<size_t>(body.size()));
        response.setBody(body);
        return;
    }

    const std::string serverPage = router.getServer() ? router.getServer()->getErrorPage(code) : "";
    if (!serverPage.empty() && readFileContent(serverPage, body)) {
        response.setStatus(code, "Error");
        response.addHeader("Content-Type", mimeTypes.get(serverPage));
        response.addHeader("Content-Length", typeToString<size_t>(body.size()));
        response.setBody(body);
        return;
    }

    // Fallback HTML
    body = generateHtml(code, msg);
    response.setStatus(code, "Error");
    response.addHeader("Content-Type", "text/html");
    response.addHeader("Content-Length", typeToString<size_t>(body.size()));
    response.setBody(body);
}
