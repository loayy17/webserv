#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <iostream>
#include "Types.hpp"
class Logger {
   public:
    static bool info(const String& message);
    static bool error(const String& message);
};
#endif