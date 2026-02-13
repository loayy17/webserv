#include "ErrorPageHandler.hpp"
#include <fstream>
#include <sstream>

std::string ErrorPageHandler::generateHtml(int code, const std::string& msg) const {
    String errorPage;

    if (!readFileContent("www/error.html", errorPage)) {
        // Fallback HTML if error.html is not found
        return "<!DOCTYPE html>\n"
               "<html lang=\"en\">\n"
               "<head>\n"
               "<meta charset=\"UTF-8\">\n"
               "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
               "<title>" +
               typeToString<int>(code) + " " + msg +
               "</title>\n"
               "</head>\n"
               "<body>\n"
               "<h1>" +
               typeToString<int>(code) + " " + msg +
               "</h1>\n"
               "<p>The server encountered an error while processing your request.</p>\n"
               "</body>\n"
               "</html>\n";
    }
    return errorPage;
}

void ErrorPageHandler::handle(HttpResponse& response, const RouteResult& resultRouter, const MimeTypes& mimeTypes) const {
    int         code = resultRouter.getStatusCode() ? resultRouter.getStatusCode() : 500;
    std::string msg  = resultRouter.getErrorMessage().empty() ? "Error" : resultRouter.getErrorMessage();

    std::string body;
    // Check for custom error page in location first, then server
    const std::string customPage = resultRouter.getLocation() ? resultRouter.getLocation()->getErrorPage(code) : "";
    if (!customPage.empty() && readFileContent(customPage, body)) {
        response.setStatus(code, "Error");
        response.addHeader("Content-Type", mimeTypes.get(customPage));
        response.addHeader("Content-Length", typeToString<size_t>(body.size()));
        response.setBody(body);
        return;
    }

    const std::string serverPage = resultRouter.getServer() ? resultRouter.getServer()->getErrorPage(code) : "";
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
