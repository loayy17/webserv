#include "ConfigParser.hpp"

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

ConfigParser::~ConfigParser() {
    servers.clear();
    lines.clear();
}

bool ConfigParser::getNextLine(String& out) {
    if (curr_index >= lines.size())
        return false;
    out = lines[curr_index++];
    return true;
}

bool ConfigParser::parse() {
    String line;
    bool   is_http_defined = false;
    while (getNextLine(line)) {
        // TODO Make support single and double quotes
        if (line == "http {" || line == "\"http\" {") {
            if (is_http_defined)
                return Logger::error("only one http block allowed");
            is_http_defined = true;
            if (scope != NONE)
                return Logger::error("http block in invalid position");
            scope = HTTP;
            if (!parseHttp())
                return false;
            scope = NONE;
        } else if (line == "server {" || line == "\"server\" {") {
            if (scope != NONE)
                return Logger::error("server block in invalid position");
            scope = SERVER;
            if (!parseServer())
                return false;
            scope = NONE;
        } else
            return Logger::error("Invalid directive: " + line);
    }
    return validate();
}

bool ConfigParser::parseHttp() {
    String line;
    while (getNextLine(line)) {
        String t = trimSpacesComments(line);
        if (t.empty())
            continue;
        if (t == "}") {
            scope = NONE;
            break;
        }
        if (t == "server {" || t == "\"server\" {") {
            if (!parseServer())
                return false;
            continue;
        }
        String       key;
        VectorString values;
        if (!parseKeyValue(t, key, values))
            return Logger::error("invalid http directive: " + t);

        if (key == "client_max_body_size") {
            if (!httpClientMaxBody.empty())
                return Logger::error("duplicate client_max_body_size");
            httpClientMaxBody = values[0];
        } else
            return Logger::error("Unknown http directive: " + key);
    }
    if (servers.size() == 0) {
        return Logger::error("No server defined");
    }
    if (scope != NONE)
        return Logger::error("Unexpected end of file, missing '}' in http block");
    return true;
}

ServerDirectiveMap ConfigParser::getServerDirectives() {
    static ServerDirectiveMap m;

    m["listen"]               = &ServerConfig::setListen;
    m["server_name"]          = &ServerConfig::setServerName;
    m["root"]                 = &ServerConfig::setRoot;
    m["index"]                = &ServerConfig::setIndexes;
    m["client_max_body_size"] = &ServerConfig::setClientMaxBody;
    m["error_page"]           = &ServerConfig::setErrorPage;

    return m;
}

bool ConfigParser::parseServer() {
    ServerConfig srv;
    scope = SERVER;

    String l;
    serverDirectives = getServerDirectives();

    while (getNextLine(l)) {
        String t = trimSpacesComments(l);
        if (t.empty())
            continue;
        if (t == "}") {
            scope = HTTP;
            break;
        }
        if ((t.find("location") == 0 || t.find("\"location\"") == 0) && t[t.size() - 1] == '{') {
            if (!parseLocation(srv, t))
                return false;
            continue;
        }

        if (!parseServerDirective(t, srv))
            return false;
    }
    if (srv.getListenAddresses().empty())
        return Logger::error("server missing listen directive");
    if (srv.getLocations().empty())
        return Logger::error("at least one location is required");
    servers.push_back(srv);
    if (scope != HTTP)
        return Logger::error("Unexpected end of file, missing '}' in server block");
    return true;
}

bool ConfigParser::parseServerDirective(const String& l, ServerConfig& srv) {
    String       key;
    VectorString values;
    if (!parseKeyValue(l, key, values))
        return Logger::error("invalid server directive: " + l);
    if (serverDirectives.find(key) == serverDirectives.end())
        return Logger::error("Unknown server directive: " + key);
    ServerDirectiveMap::const_iterator it = serverDirectives.find(key);
    return (srv.*(it->second))(values);
}

bool ConfigParser::parseLocation(ServerConfig& srv, const String& header) {
    String       loc;
    VectorString values;
    locationDirectives = getLocationDirectives();
    if (!parseKeyValue(header, loc, values))
        return Logger::error("invalid location syntax");
    if (values.size() != 2 || values[1] != "{")
        return Logger::error("Invalid location syntax");
    if (values.empty() || values[0][0] != '/')
        return Logger::error("Location path required");
    for (size_t i = 0; i < srv.getLocations().size(); i++) {
        if (srv.getLocations()[i].getPath() == values[0])
            return Logger::error("duplicate location path: " + values[0]);
    }

    LocationConfig locCfg(values[0]);
    scope = LOCATION;

    String line;
    while (getNextLine(line)) {
        if (line == "}") {
            scope = SERVER;
            break;
        } else if (!parseLocationDirective(line, locCfg))
            return false;
    }
    srv.addLocation(locCfg);
    if (scope != SERVER)
        return Logger::error("Unexpected end of file, missing '}' in location block");
    return true;
}

LocationDirectiveMap ConfigParser::getLocationDirectives() {
    static LocationDirectiveMap m;

    m["root"]                 = &LocationConfig::setRoot;
    m["autoindex"]            = &LocationConfig::setAutoIndex;
    m["index"]                = &LocationConfig::setIndexes;
    m["client_max_body_size"] = &LocationConfig::setClientMaxBody;
    m["methods"]              = &LocationConfig::setAllowedMethods;
    m["return"]               = &LocationConfig::setRedirect;
    m["cgi_pass"]             = &LocationConfig::setCgiPass;
    m["upload_dir"]           = &LocationConfig::setUploadDir;
    m["error_page"]           = &LocationConfig::setErrorPage;

    return m;
}

bool ConfigParser::parseLocationDirective(const String& l, LocationConfig& loc) {
    String       key;
    VectorString values;
    if (!parseKeyValue(l, key, values))
        return Logger::error("invalid location directive: " + l);
    if (locationDirectives.find(key) == locationDirectives.end())
        return Logger::error("Unknown location directive: " + key);
    LocationDirectiveMap::const_iterator it = locationDirectives.find(key);
    return (loc.*(it->second))(values);
}

bool ConfigParser::validate() {
    if (servers.empty())
        return Logger::error("No server defined");

    for (size_t i = 0; i < servers.size(); i++) {
        ServerConfig& s = servers[i];
        if (httpClientMaxBody.empty() && s.getClientMaxBody().empty()) {
            httpClientMaxBody = "1M";
            s.setClientMaxBody("1M");
        }
        if (!httpClientMaxBody.empty() && s.getClientMaxBody().empty())
            s.setClientMaxBody(httpClientMaxBody);
        VectorLocationConfig& locs = s.getLocations();
        for (size_t j = 0; j < locs.size(); j++) {
            if (locs[j].getRoot().empty() && !locs[j].getIsRedirect()) {
                if (s.getRoot().empty())
                    return Logger::error("location has no root and server has no root");
                locs[j].setRoot(s.getRoot());
            }
            if (locs[j].getAllowedMethods().empty())
                locs[j].addAllowedMethod("GET");
            if (locs[j].getClientMaxBody().empty())
                locs[j].setClientMaxBody(s.getClientMaxBody());
            if (locs[j].getIndexes().empty()) {
                if (s.getIndexes().empty())
                    locs[j].setIndexes(VectorString(1, "index.html"));
                else
                    locs[j].setIndexes(s.getIndexes());
            }
        }
    }

    return true;
}

VectorServerConfig ConfigParser::getServers() const {
    return servers;
}

String ConfigParser::getHttpClientMaxBody() const {
    return httpClientMaxBody;
}
