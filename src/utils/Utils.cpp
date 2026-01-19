#include "Utils.hpp"

std::string toUpperWords(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] >= 'a' && result[i] <= 'z')
            result[i] = std::toupper(result[i]);
    }
    return result;
}
std::string toString(size_t _value) {
    std::ostringstream oss;
    oss << _value;
    return oss.str();
}
std::string toString(ssize_t _value) {
    std::ostringstream oss;
    oss << _value;
    return oss.str();
}
std::string toString(int _value) {
    std::ostringstream oss;
    oss << _value;
    return oss.str();
}
