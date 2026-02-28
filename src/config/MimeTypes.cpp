#include "MimeTypes.hpp"
MimeTypes::MimeTypes() {
    VectorString lines;
    if (!convertFileToLines("src/config/mime.types", lines))
        mimeTypesMap["application/octet-stream"] = "";
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
        return "application/octet-stream";
    String found = findValueStrInMap(mimeTypesMap, extention);
    if (found.empty())
        return "application/octet-stream";
    return found;
}

MimeTypes::MimeTypes(const MimeTypes& other) : mimeTypesMap(other.mimeTypesMap) {}

MimeTypes& MimeTypes::operator=(const MimeTypes& other) {
    if (this != &other) {
        mimeTypesMap = other.mimeTypesMap;
    }
    return *this;
}