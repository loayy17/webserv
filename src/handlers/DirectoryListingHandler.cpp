#include "DirectoryListingHandler.hpp"

DirectoryListingHandler::DirectoryListingHandler() {}
DirectoryListingHandler::DirectoryListingHandler(const DirectoryListingHandler& other) {
    (void)other;
}
DirectoryListingHandler& DirectoryListingHandler::operator=(const DirectoryListingHandler& other) {
    (void)other;
    return *this;
}
DirectoryListingHandler::~DirectoryListingHandler() {}

bool DirectoryListingHandler::readDirectoryEntries(const String& path, const String& uri, std::vector<FileHandler>& entries) const {
    DIR* dir = opendir(path.c_str());
    if (!dir)
        return Logger::error("Failed to open directory: " + path);
    struct dirent* entry;
    // add . and .. entries for navigation
    entries.push_back(FileHandler(NULL, ".", uri));  // . entry
    entries.push_back(FileHandler(NULL, "..", uri)); // .. entry
    while ((entry = readdir(dir)) != NULL) {
        String filename = entry->d_name;
        if (filename[0] == '.')
            continue;
        entries.push_back(FileHandler(entry, path, uri));
    }
    if (closedir(dir) == -1)
        return Logger::error("Failed to close directory: " + path);

    return true;
}

String DirectoryListingHandler::buildRow(const FileHandler& fileInfo) const {
    return "<tr class=\"row\">\n"
           "<td class=\"icon\"><img src=\"" +
           fileInfo.getIcon() +
           "\"/></td>\n"
           "<td class=\"name\"><a href=\"" +
           fileInfo.getFileLink() + "\">" + fileInfo.getFileName() +
           "</a></td>\n"
           "<td class=\"date\">" +
           fileInfo.getLastModifiedDate() +
           "</td>\n"
           "<td class=\"size\">" +
           fileInfo.getSize() +
           "</td>\n"
           "<td></td>\n"
           "</tr>\n";
}

bool DirectoryListingHandler::handle(const RouteResult& resultRouter, HttpResponse& response) const {
    std::vector<FileHandler> entries;
    String                   path = resultRouter.getPathRootUri();
    String                   uri  = resultRouter.getRequest().getUri();
    if (path.empty() || !readDirectoryEntries(path, uri, entries))
        return false;
    if (uri.size() > 1 && uri[uri.size() - 1] == '/')
        uri = uri.substr(0, uri.size() - 1);
    uri = htmlEntities(uri);
    String html =
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "<title>Index of " +
        uri +
        "</title>\n"
        "<style>\n"
        "body{font-family:system-ui;background:#0f172a;color:#e2e8f0;margin:0;padding:40px;}\n"
        "h1{font-weight:600;margin-bottom:20px;}\n"
        "table{width:100%;border-collapse:collapse;background:#020617;border-radius:12px;overflow:hidden;}\n"
        "thead{background:#020617;}\n"
        "th{font-size:13px;text-align:left;padding:14px;color:#94a3b8;border-bottom:1px solid #1e293b;}\n"
        "td{padding:14px;border-bottom:1px solid #0f172a;}\n"
        ".row:hover{background:#020617;}\n"
        ".icon img{width:20px;height:20px;}\n"
        ".name a{color:#38bdf8;text-decoration:none;font-weight:500;}\n"
        ".name a:hover{text-decoration:underline;}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<h1>Index of " +
        uri +
        "</h1>\n"
        "<table>\n"
        "<thead>\n"
        "<tr>\n"
        "<th style=\"width:40px\"></th>\n"
        "<th>Name</th>\n"
        "<th>Last Modified</th>\n"
        "<th>Size</th>\n"
        "<th></th>\n"
        "</tr>\n"
        "</thead>\n"
        "<tbody>\n";

    for (size_t i = 0; i < entries.size(); ++i)
        html += !entries[i].getFileName().empty() ? buildRow(entries[i]) : "";

    html += "</tbody>\n</table>\n</body>\n</html>";

    response.setStatus(HTTP_OK, "OK");
    response.setResponseHeaders("text/html", html.size());
    if (resultRouter.getRequest().getMethod() != "HEAD")
        response.setBody(html);
    response.addHeader("Server", "Webserv/1.0");
    return true;
}
