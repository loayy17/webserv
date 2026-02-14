#ifndef UTILS_HPP
#define UTILS_HPP
#include <sys/stat.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "Constants.hpp"
#include "Enums.hpp"
#include "Logger.hpp"
#include "Types.hpp"

// Time methods
time_t getCurrentTime();
void   updateTime(time_t& t);
time_t getDifferentTime(const time_t& start, const time_t& end);
String formatTime(time_t t);
// String methods
String toUpperWords(const String& str);
String toLowerWords(const String& str);
String trimSpaces(const String& s);
String trimQuotes(const String& s);
String trimSpacesComments(const String& s);
String cleanCharEnd(const String& v, char c);
bool   splitByChar(const String& line, String& key, String& value, char endChar, bool reverse = false);
bool   splitByString(const String& line, VectorString& values, const String& delimiter);

// File methods
bool        convertFileToLines(String file, VectorString& lines);
bool        readFileContent(const String& filePath, String& content);
bool        fileExists(const String& path);
struct stat getFileStat(const String& path);
FileType    getFileType(const struct stat& st);
FileType    getFileType(const String& path);
String      sanitizeFilename(const String& filename);
String      htmlEntities(const String& str);
bool        ensureDirectoryExists(const String& dirPath);
String      extractFileExtension(const String& path);
String      extractFilenameFromHeader(const String& contentDisposition);
bool        parseMultipartFormData(const String& body, const String& boundary, String& filename, String& fileContent);
String      extractBoundaryFromContentType(const String& contentType);
// vector methods
bool checkAllowedMethods(const String& m);
bool parseKeyValue(const String& line, String& key, VectorString& values);

String findValueStrInMap(const MapString& map, const String& key);
String findValueIntInMap(const MapIntString& map, int key);
// Path methods
size_t convertMaxBodySize(const String& maxBody);
String formatSize(double size);
String normalizePath(const String& path);
String joinPaths(const String& firstPath, const String& secondPath);
bool   pathStartsWith(const String& path, const String& prefix);

String generateGUID();
// Map methods
template <typename MapType, typename KeyType>
bool keyExists(const MapType& m, const KeyType& key) {
    typename MapType::const_iterator it = m.find(key);
    return it != m.end();
}
// find value in map
template <typename MapType, typename KeyType, typename ValueType>
ValueType getValue(const MapType& m, const KeyType& key, const ValueType& defaultValue = ValueType()) {
    typename MapType::const_iterator it = m.find(key);
    if (it != m.end()) {
        return it->second;
    }
    return defaultValue;
}
template <typename MapType, typename KeyType>
bool hasNonEmptyValue(const MapType& m, const KeyType& key) {
    typename MapType::const_iterator it = m.find(key);
    return (it != m.end() && !it->second.empty());
}
// Type conversion methods
template <typename type>
String typeToString(type _value) {
    std::stringstream ss;
    ss << _value;
    return ss.str();
}
template <typename type>
type stringToType(const String& str) {
    std::stringstream ss(str);
    type              value;
    ss >> value;
    return value;
}

template <typename K, typename V>
bool isKeyInMap(const K& key, const std::map<K, V>& m) {
    return m.find(key) != m.end();
}

template <typename K, typename V>
bool isKeyInVector(const K& key, const std::vector<K, V>& vector) {
    for (size_t i = 0; i < vector.size(); ++i)
        if (vector[i] == key)
            return true;
    return false;
}

#endif