#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "../config/LocationConfig.hpp"
#include "../config/ServerConfig.hpp"
#include "../utils/Utils.hpp"
#include "ConfigLexer.hpp"

class ConfigParser {
   public:
    ConfigParser(const String& filename);
    ~ConfigParser();

    bool                      parse();
    const VectorServerConfig& getServers() const;
    const ssize_t&            getHttpClientMaxBody() const;

   private:
    ConfigLexer        _lexer;
    ConfigToken        _current;
    bool               _haveHttp;
    VectorServerConfig _servers;
    ssize_t            _httpClientMaxBody;

    // Directive maps – initialised in constructor
    ServerDirectiveMap   _serverDirectives;
    LocationDirectiveMap _locationDirectives;

    void nextToken();
    bool error(const String& msg);
    bool expect(Type type, const String& expectedDesc);

    bool parseHttp();
    bool parseServer();
    bool parseLocation(ServerConfig& srv);
    bool validate();
};

#endif