#ifndef UPLOADER_HANDLER_HPP
#define UPLOADER_HANDLER_HPP

#include <string>
#include "../http/HttpResponse.hpp"

class UploaderHandler {
   public:
    UploaderHandler();
    UploaderHandler(const UploaderHandler& other);
    UploaderHandler& operator=(const UploaderHandler& other);
    ~UploaderHandler();

    bool handle(const String& uploadDir, const String& filename, const String& content, HttpResponse& response) const;
    
};

#endif
