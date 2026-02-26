#include "FileHandler.hpp"
FileHandler::FileHandler() : name(), link(), icon(), size(), lastModified() {}
FileHandler::FileHandler(struct dirent* entry, const String& basePath, String pathUri) {
    name         = "";
    link         = "";
    size         = "-";
    lastModified = "";
    bool isRoot  = (pathUri == "/" || pathUri.empty());
    if (!entry) {
        if (isRoot)
            return; // skip
        name = basePath == "." ? "Go To Parent" : "Back";
        link = basePath == "." ? "/" : "../";
        icon = basePath == "." ? "    https://cdn-icons-png.flaticon.com/512/17578/17578298.png"
                               : "https://cdn-icons-png.flaticon.com/512/7945/7945195.png";
        return;
    }
    String fileName = entry->d_name;

    name             = htmlEntities(fileName);
    link             = htmlEntities(fileName);
    icon             = "https://cdn-icons-png.flaticon.com/512/9166/9166568.png"; // UNKnown
    struct stat st   = getFileStat(joinPaths(basePath, fileName));
    FileType    type = getFileType(st);
    if (type == DIRECTORY) {
        link += "/";
        icon = "https://cdn-icons-png.flaticon.com/512/10536/10536917.png";
    } else if (type == SINGLEFILE) {
        lastModified = formatDateTime(st.st_mtime);
        size         = formatSize(st.st_size);
        icon         = "https://cdn-icons-png.flaticon.com/512/1091/1091007.png";
    }
}

FileHandler::FileHandler(const FileHandler& other)
    : name(other.name), link(other.link), icon(other.icon), size(other.size), lastModified(other.lastModified) {}
FileHandler& FileHandler::operator=(const FileHandler& other) {
    if (this != &other) {
        name         = other.name;
        link         = other.link;
        icon         = other.icon;
        size         = other.size;
        lastModified = other.lastModified;
    }
    return *this;
}
FileHandler::~FileHandler() {}

String FileHandler::getFileName() const {
    return name;
}
String FileHandler::getFileLink() const {
    return link;
}
String FileHandler::getIcon() const {
    return icon;
}
String FileHandler::getSize() const {
    return size;
}
String FileHandler::getLastModifiedDate() const {
    return lastModified;
}