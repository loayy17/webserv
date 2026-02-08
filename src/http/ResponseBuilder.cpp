#include "ResponseBuilder.hpp"

ResponseBuilder::ResponseBuilder() : mimeTypes() {}
ResponseBuilder::ResponseBuilder(const MimeTypes& _mimeTypes) : mimeTypes(_mimeTypes) {}
ResponseBuilder::~ResponseBuilder() {}

HandlerType getHandlerType(const Router& router) {
    if (router.isUploadRequest(router.getRequest().getMethod(), *router.getLocation()))
        return UPLOAD;
    if (router.isCgiRequest(router.getPathRootUri(), *router.getLocation()))
        return CGI;
    if (getFileType(router.getPathRootUri()) == DIRECTORY)
        return DIRECTORY_LISTING;
    if (getFileType(router.getPathRootUri()) == SINGLEFILE)
        return STATIC;
    return NOT_FOUND;
}

HttpResponse ResponseBuilder::build(const Router& router) {
    HttpResponse response;

    // Error or path not found
    if (!router.getIsPathFound() || router.getStatusCode() != HTTP_OK) {
        handleError(response, router);
        return response;
    }
    HandlerType handleType = getHandlerType(router);
    std::cout << "Handler type: " << handleType << std::endl;
    switch (handleType) {
        case DIRECTORY_LISTING:
            if (handleDirectory(response, router))
                return response;
            break;
        case UPLOAD:
            if (handleUpload(response, router))
                return response;
            break;
        case CGI:
            if (handleCgi(response, router))
                return response;
            break;
        case STATIC:
            if (handleStatic(response, router))
                return response;
            break;
        default:
            break;
    }
    handleError(response, router);
    return response;
}

HttpResponse ResponseBuilder::buildError(int code, const std::string& msg) {
    HttpResponse response;
    Router       router(code, msg);
    handleError(response, router);
    return response;
}

bool ResponseBuilder::handleStatic(HttpResponse& response, const Router& router) const {
    StaticFileHandler filehandler(mimeTypes);
    return filehandler.handle(router, response);
}

bool ResponseBuilder::handleDirectory(HttpResponse& response, const Router& router) const {
    DirectoryListingHandler dirHandler;
    return dirHandler.handle(router, response);
}

bool ResponseBuilder::handleCgi(HttpResponse& response, const Router& router) const {
    const LocationConfig* loc = router.getLocation();
    if (!loc)
        return false;

    if (!router.isCgiRequest(router.getPathRootUri(), *loc))
        return false;

    CgiHandler cgiHandler;
    return cgiHandler.handle(router, response);
}

bool ResponseBuilder::handleUpload(HttpResponse& response, const Router& router) const {
    const LocationConfig* loc = router.getLocation();
    if (!loc)
        return false;

    if (!router.isUploadRequest(router.getRequest().getMethod(), *loc))
        return false;

    UploaderHandler uploader;
    std::cout << " handler  upload receive" << std::endl;
    
    String filename;
    String fileContent;
    
    // Check if this is a multipart/form-data upload
    String contentType = router.getRequest().getContentType();
    if (contentType.find("multipart/form-data") != String::npos) {
        // Extract boundary from Content-Type
        String boundary = extractBoundaryFromContentType(contentType);
        
        // Parse multipart body to extract filename and content
        if (boundary.empty() || !parseMultipartFormData(router.getRequest().getBody(), boundary, filename, fileContent)) {
            std::cout << "Failed to parse multipart form data" << std::endl;
            // Fallback to timestamp-based filename
            filename = "upload_" + typeToString<time_t>(getCurrentTime()) + ".dat";
            fileContent = router.getRequest().getBody();
        } else {
            std::cout << "Extracted filename: " << filename << std::endl;
        }
    } else {
        // Not multipart, try to extract from Content-Disposition header or generate default
        filename = extractFilenameFromHeader(router.getRequest().getHeader(HEADER_CONTENT_DISPOSITION));
        if (filename.empty()) {
            // Generate timestamp-based filename
            filename = "upload_" + typeToString<time_t>(getCurrentTime()) + ".dat";
        }
        fileContent = router.getRequest().getBody();
    }
    
    return uploader.handle(loc->getUploadDir(), filename, fileContent, response);
}

void ResponseBuilder::handleError(HttpResponse& response, const Router& router) {
    ErrorPageHandler handler;
    handler.handle(response, router, mimeTypes);
}

String ResponseBuilder::getErrorPagePath(const Router& router, int code) const {
    if (!router.getLocation())
        return "";

    const LocationConfig* loc = router.getLocation();

    String path = loc->getErrorPage(code);
    if (!path.empty() && getFileType(path) == SINGLEFILE)
        return path;

    path = router.getServer()->getErrorPage(code);
    if (!path.empty() && getFileType(path) == SINGLEFILE)
        return path;

    return "";
}
