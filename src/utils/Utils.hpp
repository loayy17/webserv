#ifndef UTILS_HPP
#define UTILS_HPP
#include <unistd.h>
#include <sstream>
#include <string>

std::string toUpperWords(const std::string& str);
std::string toString(size_t _value);
std::string toString(ssize_t _value);
std::string toString(int _value);

#endif