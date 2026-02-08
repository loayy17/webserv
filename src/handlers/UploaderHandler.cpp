#include <sys/stat.h>
#include <fstream>
#include "../handlers/UploaderHandler.hpp"
#include "../utils/Utils.hpp"

UploaderHandler::UploaderHandler() {}
UploaderHandler::~UploaderHandler() {}

bool UploaderHandler::handle(const String& uploadDir, const String& filename, const String& content, HttpResponse& response) const {
    if (uploadDir.empty() || filename.empty())
        return false;

    String safeName = sanitizeFilename(filename);
    String fullPath = joinPaths(uploadDir, safeName);

    // Ensure upload directory exists
    struct stat st;
    if (stat(uploadDir.c_str(), &st) == -1) {
        if (mkdir(uploadDir.c_str(), 0755) == -1)
            return false;
    }

    std::ofstream out(fullPath.c_str(), std::ios::binary);
    if (!out.is_open())
        return false;

    out.write(content.c_str(), content.size());
    out.close();

    response.setStatus(201, "Created");
    response.addHeader("Content-Type", "text/plain");
    response.addHeader("Content-Length", typeToString<size_t>(content.size()));
    response.setBody("File uploaded successfully: " + safeName);
    return true;
}
