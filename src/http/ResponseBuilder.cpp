#include "ResponseBuilder.hpp"

ResponseBuilder::ResponseBuilder() : mimeTypes() {}
ResponseBuilder::ResponseBuilder(const MimeTypes& _mimeTypes) : mimeTypes(_mimeTypes) {}
ResponseBuilder::~ResponseBuilder() {}

HandlerType getHandlerType(const RouteResult& resultRouter) {
    if (resultRouter.getIsUploadRequest())
        return UPLOAD;
    if (resultRouter.getIsCgiRequest())
        return CGI;
    String path        = resultRouter.getPathRootUri();
    String method      = resultRouter.getRequest().getMethod();
    bool   isAutoIndex = resultRouter.getLocation()->getAutoIndex();
    if (getFileType(path) == DIRECTORY && isAutoIndex && (method == "GET" || method == "HEAD"))
        return DIRECTORY_LISTING;
    if (getFileType(path) == SINGLEFILE && (method == "GET" || method == "HEAD"))
        return STATIC;
    if (getFileType(path) == SINGLEFILE && method == "DELETE")
        return DELETE_FILE;
    return NOT_FOUND;
}

HttpResponse ResponseBuilder::build(const RouteResult& resultRouter, CgiProcess* cgi, const VectorInt& openFds) {
    HttpResponse response;

    // Handle redirect
    if (resultRouter.getIsRedirect()) {
        response.setStatus(resultRouter.getStatusCode(), getHttpStatusMessage(resultRouter.getStatusCode()));
        response.addHeader("Location", resultRouter.getRedirectUrl());
        response.addHeader("Content-Length", "0");
        return response;
    }

    // Error or path not found
    if (resultRouter.getStatusCode() != HTTP_OK) {
        handleError(response, resultRouter);
        return response;
    }
    HandlerType handleType = getHandlerType(resultRouter);
    switch (handleType) {
        case DIRECTORY_LISTING:
            if (handleDirectory(response, resultRouter))
                return response;
            break;
        case UPLOAD:
            if (handleUpload(response, resultRouter))
                return response;
            break;
        case CGI:
            if (cgi && handleCgi(response, resultRouter, cgi, openFds))
                return response;
            break;
        case STATIC:
            if (handleStatic(response, resultRouter))
                return response;
            break;
        case DELETE_FILE:
            if (handleDelete(response, resultRouter))
                return response;
            break;
        default:
            break;
    }
    RouteResult errResult = resultRouter;
    if (resultRouter.getStatusCode() == HTTP_OK)
        errResult.setCodeAndMessage(HTTP_INTERNAL_SERVER_ERROR, "Internal Server Error");
    handleError(response, errResult);
    return response;
}

HttpResponse ResponseBuilder::buildError(int code, const std::string& msg) {
    HttpResponse response;
    RouteResult  resultRouter;
    resultRouter.setCodeAndMessage(code, msg);
    handleError(response, resultRouter);
    return response;
}

bool ResponseBuilder::handleStatic(HttpResponse& response, const RouteResult& resultRouter) const {
    StaticFileHandler filehandler(mimeTypes);
    return filehandler.handle(resultRouter, response);
}

bool ResponseBuilder::handleDelete(HttpResponse& response, const RouteResult& resultRouter) const {
    DeleteHandler handler;
    return handler.handle(resultRouter, response);
}

bool ResponseBuilder::handleDirectory(HttpResponse& response, const RouteResult& resultRouter) const {
    DirectoryListingHandler dirHandler;
    return dirHandler.handle(resultRouter, response);
}

bool ResponseBuilder::handleUpload(HttpResponse& response, const RouteResult& resultRouter) const {
    const LocationConfig* loc = resultRouter.getLocation();
    if (!loc || !resultRouter.getIsUploadRequest())
        return false;

    UploaderHandler uploader;

    String filename;
    String fileContent;

    // Check if this is a multipart/form-data upload
    String contentType = resultRouter.getRequest().getContentType();
    if (contentType.find("multipart/form-data") != String::npos) {
        // Extract boundary from Content-Type
        String boundary = extractBoundaryFromContentType(contentType);

        // Parse multipart body to extract filename and content
        if (boundary.empty() || !parseMultipartFormData(resultRouter.getRequest().getBody(), boundary, filename, fileContent)) {
            // Fallback to timestamp-based filename
            filename    = "upload_" + typeToString<time_t>(getCurrentTime()) + ".dat";
            fileContent = resultRouter.getRequest().getBody();
        }
    } else {
        // Not multipart, try to extract from Content-Disposition header or generate default
        filename = extractFilenameFromHeader(resultRouter.getRequest().getHeader(HEADER_CONTENT_DISPOSITION));
        if (filename.empty()) {
            // Generate timestamp-based filename
            filename = "upload_" + typeToString<time_t>(getCurrentTime()) + ".dat";
        }
        fileContent = resultRouter.getRequest().getBody();
    }

    return uploader.handle(loc->getUploadDir(), filename, fileContent, response);
}

void ResponseBuilder::handleError(HttpResponse& response, const RouteResult& resultRouter) {
    ErrorPageHandler handler;
    handler.handle(response, resultRouter, mimeTypes);
}

String ResponseBuilder::getErrorPagePath(const RouteResult& resultRouter, int code) const {
    if (!resultRouter.getLocation())
        return "";

    const LocationConfig* loc = resultRouter.getLocation();

    String path = loc->getErrorPage(code);
    if (!path.empty() && getFileType(path) == SINGLEFILE)
        return path;

    path = resultRouter.getServer()->getErrorPage(code);
    if (!path.empty() && getFileType(path) == SINGLEFILE)
        return path;

    return "";
}

bool ResponseBuilder::handleCgi(HttpResponse& response, const RouteResult& resultRouter, CgiProcess* cgi, const VectorInt& openFds) const {
    if (!cgi)
        return false;
    CgiHandler handler(*cgi);
    return handler.handle(resultRouter, response, openFds);
}

HttpResponse ResponseBuilder::buildCgiResponse(CgiProcess& cgi) {
    HttpResponse response;
    if (cgi.finish() && CgiHandler::parseOutput(cgi.getOutput(), response))
        response.addHeader(HEADER_CONTENT_LENGTH, typeToString<size_t>(response.getBody().size()));
    else
        response = buildError(HTTP_INTERNAL_SERVER_ERROR, "CGI Error");
    cgi.reset();
    return response;
}
