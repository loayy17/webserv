#include "../handlers/UploaderHandler.hpp"
#include <sys/stat.h>
#include <cstdlib>
#include <fstream>
#include "../utils/Utils.hpp"

UploaderHandler::UploaderHandler() {}
UploaderHandler::UploaderHandler(const UploaderHandler& other) {
    (void)other;
}
UploaderHandler& UploaderHandler::operator=(const UploaderHandler& other) {
    (void)other;
    return *this;
}
UploaderHandler::~UploaderHandler() {}

bool UploaderHandler::handle(const String& uploadDir, const String& filename, const String& content, HttpResponse& response) const {
    if (uploadDir.empty() || filename.empty())
        return Logger::error("uploader folder is empty");
    String safeName     = sanitizeFilename(filename);
    String fullPath     = joinPaths(uploadDir, safeName);

    // Ensure upload directory exists
    if (!ensureDirectoryExists(uploadDir))
        return Logger::error("Failed to create upload directory: " + uploadDir);

    std::ofstream out(fullPath.c_str(), std::ios::binary);
    if (!out.is_open())
        return false;

    out.write(content.c_str(), content.size());
    out.close();

    String successMsg = "File uploaded successfully: " + safeName;
    response.setStatus(HTTP_CREATED, "Created");
    response.setResponseHeaders("text/plain", successMsg.size());
    response.setBody(successMsg);
    response.addHeader(HEADER_SERVER, "Webserv/1.0");
    return true;
}
