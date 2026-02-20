#include "ConfigLexer.hpp"

ConfigLexer::ConfigLexer(const std::string& filename)
    : _line(1) {
    _file.open(filename.c_str());
    if (!_file.is_open()) {
        throw std::runtime_error("Cannot open configuration file: " + filename);
    }
}

ConfigLexer::~ConfigLexer() {
    if (_file.is_open())
        _file.close();
}

char ConfigLexer::peek() {
    return static_cast<char>(_file.peek());
}

char ConfigLexer::get() {
    return static_cast<char>(_file.get());
}

bool ConfigLexer::eof() const {
    return _file.eof();
}

void ConfigLexer::skipWhitespaceAndComments() {
    while (!eof()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            get();
            continue;
        }
        if (c == '\n') {
            get();
            ++_line;
            continue;
        }
        if (c == '#') {
            while (!eof() && get() != '\n') {}
            ++_line;
            continue;
        }
        break;
    }
}

ConfigToken ConfigLexer::readQuotedString() {
    char quote = get(); 
    std::string value;
    int startLine = _line;
    while (!eof()) {
        char c = get();
        if (c == quote) break;
        if (c == '\n') ++_line;
        value += c;
    }
    return ConfigToken(TOKEN_STRING, value, startLine);
}

ConfigToken ConfigLexer::readWord() {
    std::string word;
    int startLine = _line;
    while (!eof()) {
        char c = peek();
        if (std::isspace(static_cast<unsigned char>(c)) ||
            c == '{' || c == '}' || c == ';' || c == '#')
            break;
        word += get();
    }
    return ConfigToken(TOKEN_WORD, word, startLine);
}

ConfigToken ConfigLexer::nextToken() {
    skipWhitespaceAndComments();
    if (eof()) return ConfigToken(TOKEN_EOF, "", _line);
    char c = peek();
    if (c == '{') { get(); return ConfigToken(TOKEN_LBRACE, "{", _line); }
    if (c == '}') { get(); return ConfigToken(TOKEN_RBRACE, "}", _line); }
    if (c == ';') { get(); return ConfigToken(TOKEN_SEMICOLON, ";", _line); }
    if (c == '"' || c == '\'') return readQuotedString();
    return readWord();
}