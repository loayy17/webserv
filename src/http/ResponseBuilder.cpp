#include "ResponseBuilder.hpp"

ResponseBuilder::ResponseBuilder() : mimeTypes() {}
ResponseBuilder::ResponseBuilder(const MimeTypes& _mimeTypes) : mimeTypes(_mimeTypes) {}
ResponseBuilder::ResponseBuilder(const ResponseBuilder& other) : mimeTypes(other.mimeTypes) {}
ResponseBuilder& ResponseBuilder::operator=(const ResponseBuilder& other) {
    if (this != &other) {
        mimeTypes = other.mimeTypes;
    }
    return *this;
}
ResponseBuilder::~ResponseBuilder() {}

HttpResponse ResponseBuilder::build(const RouteResult& resultRouter, CgiProcess* cgi, const VectorInt& openFds) {
    HttpResponse response;

    if (resultRouter.getIsRedirect()) {
        response.setStatus(resultRouter.getStatusCode(), getHttpStatusMessage(resultRouter.getStatusCode()));
        response.addHeader("Location", resultRouter.getRedirectUrl());
        response.addHeader("Content-Length", "0");
        return response;
    }

    if (resultRouter.getStatusCode() != HTTP_OK) {
        handleError(response, resultRouter);
        return response;
    }

    bool handled = false;
    switch (resultRouter.getHandlerType()) {
        case DIRECTORY_LISTING: handled = handleDirectory(response, resultRouter); break;
        case UPLOAD:            handled = handleUpload(response, resultRouter);    break;
        case CGI:               handled = cgi && handleCgi(response, resultRouter, cgi, openFds); break;
        case STATIC:            handled = handleStatic(response, resultRouter);   break;
        case DELETE_FILE:       handled = handleDelete(response, resultRouter);   break;
        default:                break;
    }

    if (!handled) {
        RouteResult errResult = resultRouter;
        if (resultRouter.getStatusCode() == HTTP_OK) {
            if (resultRouter.getHandlerType() == NOT_FOUND && isMethodWithBody(resultRouter.getRequest().getMethod()))
                errResult.setCodeAndMessage(HTTP_METHOD_NOT_ALLOWED, getHttpStatusMessage(HTTP_METHOD_NOT_ALLOWED));
            else
                errResult.setCodeAndMessage(HTTP_INTERNAL_SERVER_ERROR, "Internal Server Error");
        }
        handleError(response, errResult);
    }
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
    if (resultRouter.getStatusCode() == HTTP_METHOD_NOT_ALLOWED && resultRouter.getLocation()) {
        const VectorString& methods = resultRouter.getLocation()->getAllowedMethods();
        String              allow;
        for (size_t i = 0; i < methods.size(); ++i) {
            if (i > 0)
                allow += ", ";
            allow += methods[i];
        }
        response.addHeader("Allow", allow);
    }
}

bool ResponseBuilder::handleCgi(HttpResponse& response, const RouteResult& resultRouter, CgiProcess* cgi, const VectorInt& openFds) const {
    if (!cgi)
        return false;
    CgiHandler handler(*cgi);
    return handler.handle(resultRouter, response, openFds);
}

HttpResponse ResponseBuilder::buildCgiResponse(CgiProcess& cgi) {
    HttpResponse response;
    if (CgiHandler::parseOutput(cgi.getOutput(), response))
        response.addHeader(HEADER_CONTENT_LENGTH, typeToString<size_t>(response.getBody().size()));
    else
        response = buildError(HTTP_INTERNAL_SERVER_ERROR, "CGI Error");
    cgi.reset();
    return response;
}
