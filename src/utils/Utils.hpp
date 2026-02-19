#ifndef UTILS_HPP
#define UTILS_HPP

#include <fcntl.h>
#include <sys/stat.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include "Constants.hpp"
#include "Enums.hpp"
#include "Logger.hpp"
#include "Types.hpp"

// --- Time Methods ---
time_t getCurrentTime();
void   updateTime(time_t& t);
time_t getDifferentTime(const time_t& start, const time_t& end);
String formatTime(time_t t);

// --- String Methods ---
String toUpperWords(const String& str);
String toLowerWords(const String& str);
String trimSpaces(const String& s);
String trimQuotes(const String& s);
String trimSpacesComments(const String& s);
String cleanCharEnd(const String& v, char c);
bool   splitByChar(const String& line, String& key, String& value, char endChar, bool reverse = false);
bool   splitByString(const String& line, VectorString& values, const String& delimiter);
String htmlEntities(const String& str);
String generateGUID();

// --- Map Helpers ---
String findValueStrInMap(const MapString& map, const String& key);
String findValueIntInMap(const MapIntString& map, int key);

// --- File System Methods ---
bool        convertFileToLines(const String& file, VectorString& lines);
bool        readFileContent(const String& filePath, String& content);
bool        fileExists(const String& path);
struct stat getFileStat(const String& path);
FileType    getFileType(const struct stat& st);
FileType    getFileType(const String& path);
String      sanitizeFilename(const String& filename);
bool        ensureDirectoryExists(const String& dirPath);
String      extractFileExtension(const String& path);
String      extractDirectoryFromPath(const String& path);

// --- Path Methods ---
String normalizePath(const String& path);
String joinPaths(const String& firstPath, const String& secondPath);
bool   pathStartsWith(const String& path, const String& prefix);
String getUriRemainder(const String& uri, const String& locPath);

// --- HTTP/Network Helpers ---
bool   checkAllowedMethods(const String& m);
bool   parseKeyValue(const String& line, String& key, VectorString& values);
size_t convertMaxBodySize(const String& maxBody);
String formatSize(double size);
bool   setNonBlocking(int fd);
String getHttpStatusMessage(int code);

// --- Header/Body Parsing ---
String extractFilenameFromHeader(const String& contentDisposition);
String extractBoundaryFromContentType(const String& contentType);
bool   parseMultipartFormData(const String& body, const String& boundary, String& filename, String& fileContent);
bool   isChunkedTransferEncoding(const String& headers);
bool   decodeChunkedBody(const String& chunkedBody, String& decodedBody);
bool   getHeaderValue(const String& headers, const String& headerName, String& outValue);
size_t extractContentLength(const String& headers);

// --- Templates ---

// Check if key exists in map
template <typename MapType, typename KeyType>
bool keyExists(const MapType& m, const KeyType& key) {
    return m.find(key) != m.end();
}

// Get value from map with default
template <typename MapType, typename KeyType, typename ValueType>
ValueType getValue(const MapType& m, const KeyType& key, const ValueType& defaultValue = ValueType()) {
    typename MapType::const_iterator it = m.find(key);
    if (it != m.end()) {
        return it->second;
    }
    return defaultValue;
}

// Check if map has non-empty value for key
template <typename MapType, typename KeyType>
bool hasNonEmptyValue(const MapType& m, const KeyType& key) {
    typename MapType::const_iterator it = m.find(key);
    return (it != m.end() && !it->second.empty());
}

// Convert any streamable type to String
template <typename type>
String typeToString(type _value) {
    std::stringstream ss;
    ss << _value;
    return ss.str();
}

// Convert String to any streamable type
template <typename type>
type stringToType(const String& str) {
    std::stringstream ss(str);
    type              value;
    ss >> value;
    return value;
}

// Check if key exists in std::map (Redundant with keyExists but kept for compatibility)
template <typename K, typename V>
bool isKeyInMap(const K& key, const std::map<K, V>& m) {
    return m.find(key) != m.end();
}

// Check if key exists in std::vector
template <typename K, typename V>
bool isKeyInVector(const K& key, const std::vector<K, V>& vector) {
    for (size_t i = 0; i < vector.size(); ++i)
        if (vector[i] == key)
            return true;
    return false;
}
String urlDecode(const String& input);
#endif