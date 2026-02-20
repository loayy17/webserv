#ifndef CONFIG_LEXER_HPP
#define CONFIG_LEXER_HPP

#include "../utils/Utils.hpp"
#include "ConfigToken.hpp"

class ConfigLexer {
   public:
    ConfigLexer(const std::string& filename);
    ~ConfigLexer();

    ConfigToken nextToken();

   private:
    std::ifstream _file;
    int           _line;

    char peek();
    char get();
    bool eof() const;

    void skipWhitespaceAndComments();

    ConfigToken readQuotedString();
    ConfigToken readWord();
};

#endif