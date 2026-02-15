#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"
#include "LocationConfig.hpp"
#include "ServerConfig.hpp"

class ConfigParser {
   public:
    ConfigParser();
    ConfigParser(const ConfigParser& other);
    ConfigParser& operator=(const ConfigParser& other);
    ConfigParser(const String& f);
    ~ConfigParser();

    bool               parse();
    String             getHttpClientMaxBody() const;
    VectorServerConfig getServers() const;

   private:
    String                    file;
    std::vector<ServerConfig> servers;
    ScopeConfig               scope;
    size_t                    curr_index;
    String                    httpClientMaxBody;
    VectorString              lines;
    ServerDirectiveMap        serverDirectives;
    LocationDirectiveMap      locationDirectives;

    bool getNextLine(String& out);
    ServerDirectiveMap   getServerDirectives();
    LocationDirectiveMap getLocationDirectives();
    bool parseHttp();
    bool parseServer();
    bool parseLocation(ServerConfig& srv, const String& header);
    bool parseServerDirective(const String& l, ServerConfig& srv);
    bool parseLocationDirective(const String& l, LocationConfig& loc);
    bool validate();
};

#endif
