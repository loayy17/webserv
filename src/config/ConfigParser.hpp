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
    ConfigParser(const std::string& f);
    ~ConfigParser();

    bool               parse();
    std::string        getHttpClientMaxBody() const;
    VectorServerConfig getServers() const;

   private:
    enum Scope { NONE, HTTP, SERVER, LOCATION };

    std::string               file;
    std::vector<ServerConfig> servers;
    Scope                     scope;
    size_t                    curr_index;
    std::string               httpClientMaxBody;
    VectorString              lines;
    ServerDirectiveMap        serverDirectives;
    LocationDirectiveMap      locationDirectives;

    bool getNextLine(std::string& out);

    ServerDirectiveMap   getServerDirectives();
    LocationDirectiveMap getLocationDirectives();

    bool parseHttp();
    bool parseServer();
    bool parseLocation(ServerConfig& srv, const std::string& header);
    bool parseServerDirective(const std::string& l, ServerConfig& srv);
    bool parseLocationDirective(const std::string& l, LocationConfig& loc);
    bool validate();
};

#endif
