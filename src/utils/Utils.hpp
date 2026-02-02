#ifndef UTILS_HPP
#define UTILS_HPP
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "Constants.hpp"
#include "Logger.hpp"
#include "Types.hpp"

// Time methods
time_t getCurrentTime();
void   updateTime(time_t& t);
time_t getDifferentTime(const time_t& start, const time_t& end);
// String methods
std::string toUpperWords(const std::string& str);
std::string toLowerWords(const std::string& str);
std::string trimSpaces(const std::string& s);
std::string trimSpacesComments(const std::string& s);
std::string cleanCharEnd(const std::string& v, char c);
bool        splitByChar(const std::string& line, std::string& key, std::string& value, char endChar);
bool        splitByString(const std::string& line, VectorString& values, const std::string& delimiter);
// vector methods
bool convertFileToLines(std::string file, VectorString& lines);
bool checkAllowedMethods(const std::string& m);
bool isStringInVector(const std::string& toFind, const VectorString& fromFind);
bool parseKeyValue(const std::string& line, std::string& key, VectorString& values);

std::string findValueInVector(const VectorString& map, const std::string& key);
// Path methods
size_t      convertMaxBodySize(const std::string& maxBody);
std::string normalizePath(const std::string& path);
bool        pathStartsWith(const std::string& path, const std::string& prefix);

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
std::string typeToString(type _value) {
    std::stringstream ss;
    ss << _value;
    return ss.str();
}
template <typename type>
type stringToType(const std::string& str) {
    std::stringstream ss(str);
    type              value;
    ss >> value;
    return value;
}

#endif