#ifndef MIME_TYPES_HPP
#define MIME_TYPES_HPP
#include <iostream>
#include "../utils/Utils.hpp"
class MimeTypes {
   public:
    MimeTypes();
    MimeTypes(const MimeTypes& other);
    MimeTypes& operator=(const MimeTypes& other);
    ~MimeTypes();
    String get(const String& path) const;

   private:
    MapString mimeTypesMap;
};
#endif
