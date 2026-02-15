#include "ConfigParser.hpp"

// ============================================================================
// Constructors and Destructor
// ============================================================================

ConfigParser::ConfigParser()
    : file(""), servers(), scope(NONE), curr_index(0), httpClientMaxBody(""), lines(), serverDirectives(), locationDirectives() {}

ConfigParser::ConfigParser(const ConfigParser& other)
    : file(other.file),
      servers(other.servers),
      scope(other.scope),
      curr_index(other.curr_index),
      httpClientMaxBody(other.httpClientMaxBody),
      lines(other.lines),
      serverDirectives(other.serverDirectives),
      locationDirectives(other.locationDirectives) {}

ConfigParser& ConfigParser::operator=(const ConfigParser& other) {
    if (this != &other) {
        file               = other.file;
        servers            = other.servers;
        scope              = other.scope;
        curr_index         = other.curr_index;
        httpClientMaxBody  = other.httpClientMaxBody;
        lines              = other.lines;
        serverDirectives   = other.serverDirectives;
        locationDirectives = other.locationDirectives;
    }
    return *this;
}

ConfigParser::ConfigParser(const String& f) : file(f), scope(NONE), curr_index(0) {
    if (!convertFileToLines(file, lines))
        Logger::error("Failed to read configuration file: " + file);
}

ConfigParser::~ConfigParser() {}

// ============================================================================
// Core Helpers
// ============================================================================

bool ConfigParser::getNextLine(String& out) {
    if (curr_index >= lines.size())
        return false;
    out = lines[curr_index++];
    return true;
}

// ============================================================================
// Directive Maps
// ============================================================================

ServerDirectiveMap ConfigParser::getServerDirectives() {
    static ServerDirectiveMap m;
    if (m.empty()) {
        m["listen"]               = &ServerConfig::setListen;
        m["server_name"]          = &ServerConfig::setServerName;
        m["root"]                 = &ServerConfig::setRoot;
        m["index"]                = &ServerConfig::setIndexes;
        m["client_max_body_size"] = &ServerConfig::setClientMaxBody;
        m["error_page"]           = &ServerConfig::setErrorPage;
    }
    return m;
}

LocationDirectiveMap ConfigParser::getLocationDirectives() {
    static LocationDirectiveMap m;
    if (m.empty()) {
        m["root"]                 = &LocationConfig::setRoot;
        m["autoindex"]            = &LocationConfig::setAutoIndex;
        m["index"]                = &LocationConfig::setIndexes;
        m["client_max_body_size"] = &LocationConfig::setClientMaxBody;
        m["methods"]              = &LocationConfig::setAllowedMethods;
        m["return"]               = &LocationConfig::setRedirect;
        m["cgi_pass"]             = &LocationConfig::setCgiPass;
        m["upload_dir"]           = &LocationConfig::setUploadDir;
        m["error_page"]           = &LocationConfig::setErrorPage;
    }
    return m;
}

// ============================================================================
// Block Parsers
// ============================================================================

bool ConfigParser::parseHttp() {
    String line;
    bool   serverFound = false;

    while (getNextLine(line)) {
        if (line == "}")
            return serverFound || Logger::error("No server defined");

        String       key;
        VectorString values;
        if (!parseKeyValue(line, key, values))
            return Logger::error("Invalid http directive: " + line);

        if (key == "server" && values.size() == 1 && values[0] == "{") {
            if (!parseServer())
                return false;
            serverFound = true;
        } else if (key == "client_max_body_size") {
            if (!httpClientMaxBody.empty())
                return Logger::error("Duplicate client_max_body_size");
            httpClientMaxBody = values[0];
        } else {
            return Logger::error("Unknown http directive: " + key);
        }
    }

    return Logger::error("Unexpected end of file in http block");
}

bool ConfigParser::parseServer() {
    ServerConfig srv;
    serverDirectives = getServerDirectives();
    String line;

    while (getNextLine(line)) {
        if (line == "}") {
            if (srv.getListenAddresses().empty())
                return Logger::error("Server missing listen directive");
            if (srv.getLocations().empty())
                return Logger::error("At least one location is required");
            servers.push_back(srv);
            return true;
        }

        String       key;
        VectorString values;
        if (!parseKeyValue(line, key, values))
            return Logger::error("Invalid server directive: " + line);

        if (key == "location" && values.size() == 2 && values[1] == "{") {
            if (!parseLocation(srv, values[0]))
                return false;
        } else {
            ServerDirectiveMap::const_iterator it = serverDirectives.find(key);
            if (it == serverDirectives.end())
                return Logger::error("Unknown server directive: " + key);
            if (!(srv.*(it->second))(values))
                return false;
        }
    }

    return Logger::error("Unexpected end of file in server block");
}

bool ConfigParser::parseLocation(ServerConfig& srv, const String& path) {
    if (path.empty() || path[0] != SLASH)
        return Logger::error("Location path must start with '/'");

    const VectorLocationConfig& locs = srv.getLocations();
    for (size_t i = 0; i < locs.size(); i++) {
        if (locs[i].getPath() == path)
            return Logger::error("Duplicate location path: " + path);
    }

    LocationConfig loc(path);
    locationDirectives = getLocationDirectives();
    String line;

    while (getNextLine(line)) {
        if (line == "}") {
            srv.addLocation(loc);
            return true;
        }
        String       key;
        VectorString values;
        if (!parseKeyValue(line, key, values))
            return Logger::error("Invalid location directive: " + line);

        LocationDirectiveMap::const_iterator it = locationDirectives.find(key);
        if (it == locationDirectives.end())
            return Logger::error("Unknown location directive: " + key);
        if (!(loc.*(it->second))(values))
            return false;
    }

    return Logger::error("Unexpected end of file in location block");
}

bool ConfigParser::parseServerDirective(const String& l, ServerConfig& srv) {
    String       key;
    VectorString values;
    if (!parseKeyValue(l, key, values))
        return Logger::error("Invalid server directive: " + l);
    ServerDirectiveMap::const_iterator it = serverDirectives.find(key);
    if (it == serverDirectives.end())
        return Logger::error("Unknown server directive: " + key);

    return (srv.*(it->second))(values);
}

bool ConfigParser::parseLocationDirective(const String& l, LocationConfig& loc) {
    String       key;
    VectorString values;
    if (!parseKeyValue(l, key, values))
        return Logger::error("Invalid location directive: " + l);

    LocationDirectiveMap::const_iterator it = locationDirectives.find(key);
    if (it == locationDirectives.end())
        return Logger::error("Unknown location directive: " + key);

    return (loc.*(it->second))(values);
}

// ============================================================================
// Main Parse
// ============================================================================

bool ConfigParser::parse() {
    String line;
    bool   httpFound = false;

    while (getNextLine(line)) {
        String       key;
        VectorString values;

        if (!parseKeyValue(line, key, values))
            return Logger::error("Invalid directive: " + line);

        if (key == "http" && values.size() == 1 && values[0] == "{") {
            if (httpFound)
                return Logger::error("Only one http block allowed");
            httpFound = true;
            if (!parseHttp())
                return false;
        } else if (key == "server" && values.size() == 1 && values[0] == "{") {
            if (!parseServer())
                return false;
        } else {
            return Logger::error("Invalid directive: " + line);
        }
    }

    return validate();
}

// ============================================================================
// Validation
// ============================================================================

bool ConfigParser::validate() {
    if (servers.empty())
        return Logger::error("No server defined");

    if (httpClientMaxBody.empty())
        httpClientMaxBody = "1M";

    for (size_t i = 0; i < servers.size(); i++) {
        ServerConfig& srv = servers[i];

        if (srv.getClientMaxBody().empty())
            srv.setClientMaxBody(httpClientMaxBody);

        VectorLocationConfig& locs = srv.getLocations();
        for (size_t j = 0; j < locs.size(); j++) {
            LocationConfig& loc = locs[j];

            if (loc.getRoot().empty() && !loc.getIsRedirect()) {
                if (srv.getRoot().empty())
                    return Logger::error("Location has no root and server has no root");
                loc.setRoot(srv.getRoot());
            }

            if (loc.getAllowedMethods().empty())
                loc.addAllowedMethod("GET");

            if (loc.getClientMaxBody().empty())
                loc.setClientMaxBody(srv.getClientMaxBody());

            if (loc.getIndexes().empty()) {
                loc.setIndexes(srv.getIndexes().empty() ? VectorString(1, "index.html") : srv.getIndexes());
            }
        }
    }

    return true;
}

// ============================================================================
// Getters
// ============================================================================

VectorServerConfig ConfigParser::getServers() const {
    return servers;
}

String ConfigParser::getHttpClientMaxBody() const {
    return httpClientMaxBody;
}