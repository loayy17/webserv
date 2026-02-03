#include "MimeTypes.hpp"

MimeTypes::MimeTypes(const std::string& extension) : extension(extension) {
    if (extension == "html" || extension == "htm")
        mimeType = "text/html";
    else if (extension == "css")
        mimeType = "text/css";
    else if (extension == "txt")
        mimeType = "text/plain";
    else if (extension == "xml")
        mimeType = "text/xml";
    else if (extension == "js")
        mimeType = "application/javascript";
    else if (extension == "json")
        mimeType = "application/json";
    else if (extension == "png")
        mimeType = "image/png";
    else if (extension == "jpg" || extension == "jpeg")
        mimeType = "image/jpeg";
    else if (extension == "gif")
        mimeType = "image/gif";
    else if (extension == "ico")
        mimeType = "image/x-icon";
    else if (extension == "svg")
        mimeType = "image/svg+xml";
    else if (extension == "webp")
        mimeType = "image/webp";
    else if (extension == "woff")
        mimeType = "font/woff";
    else if (extension == "woff2")
        mimeType = "font/woff2";
    else if (extension == "ttf")
        mimeType = "font/ttf";
    else if (extension == "pdf")
        mimeType = "application/pdf";
    else
        mimeType = "application/octet-stream";
}

MimeTypes::~MimeTypes() {}

std::string MimeTypes::getMimeType() const {
    return mimeType;
}

MimeTypes::MimeTypes() : extension(""), mimeType("application/octet-stream") {}

MimeTypes::MimeTypes(const MimeTypes& other) : extension(other.extension), mimeType(other.mimeType) {}

MimeTypes& MimeTypes::operator=(const MimeTypes& other) {
    if (this != &other) {
        extension = other.extension;
        mimeType  = other.mimeType;
    }
    return *this;
}