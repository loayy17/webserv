#include "Logger.hpp"
bool Logger::info(const String& message) {
    std::cout << "\033[34m[INFO]: " << message << "\033[0m" << std::endl;
    return true;
}
bool Logger::error(const String& message) {
    std::cerr << "\033[31m[ERROR]: " << message << "\033[0m" << std::endl;
    return false;
}