#include "MimeTypes.hpp"

MimeTypes::MimeTypes() : default_mime_type(DEFAULT_MIME_TYPE) {
    VectorString lines;
    if (!convertFileToLines("src/config/mime.types", lines))
        mimeTypesMap[default_mime_type] = EMPTY_STRING;
    for (size_t i = 0; i < lines.size(); i++) {
        String       key;
        VectorString values;
        if (!parseKeyValue(lines[i], key, values))
            continue;
        for (size_t j = 0; j < values.size(); j++) {
            mimeTypesMap[values[j]] = key;
        }
    }
    lines.clear();
}

MimeTypes::~MimeTypes() {
    mimeTypesMap.clear();
}

String MimeTypes::get(const String& path) const {
    String key, extention;
    if (!splitByChar(path, key, extention, '.', true))
        return default_mime_type;
    const String& found = findValueStrInMap(mimeTypesMap, extention);
    if (found.empty())
        return default_mime_type;
    return found;
}

MimeTypes::MimeTypes(const MimeTypes& other) : mimeTypesMap(other.mimeTypesMap), default_mime_type(other.default_mime_type) {}

MimeTypes& MimeTypes::operator=(const MimeTypes& other) {
    if (this != &other) {
        mimeTypesMap      = other.mimeTypesMap;
        default_mime_type = other.default_mime_type;
    }
    return *this;
}