#include "ConfigToken.hpp"

ConfigToken::ConfigToken() : _type(TOKEN_EOF), _value(""), _n_line(0) {}

ConfigToken::ConfigToken(Type type, const String& value, int line) : _type(type), _value(value), _n_line(line) {}

ConfigToken::ConfigToken(const ConfigToken& other) : _type(other._type), _value(other._value), _n_line(other._n_line) {}

ConfigToken& ConfigToken::operator=(const ConfigToken& other) {
    if (this != &other) {
        _type   = other._type;
        _value  = other._value;
        _n_line = other._n_line;
    }
    return *this;
}

ConfigToken::~ConfigToken() {}

Type ConfigToken::getType() const {
    return _type;
}

const String& ConfigToken::getValue() const {
    return _value;
}

int ConfigToken::getLine() const {
    return _n_line;
}