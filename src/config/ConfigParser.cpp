#include "ConfigParser.hpp"

ConfigParser::ConfigParser(const String& filename) : _lexer(filename), _haveHttp(false), _httpClientMaxBody(-1) {
    nextToken();

    // ---- Server directives ----
    _serverDirectives["listen"]               = &ServerConfig::setListen;
    _serverDirectives["server_name"]          = &ServerConfig::setServerName;
    _serverDirectives["root"]                 = &ServerConfig::setRoot;
    _serverDirectives["index"]                = &ServerConfig::setIndexes;
    _serverDirectives["client_max_body_size"] = &ServerConfig::setClientMaxBody;
    _serverDirectives["error_page"]           = &ServerConfig::setErrorPage;

    // ---- Location directives ----
    _locationDirectives["root"]                 = &LocationConfig::setRoot;
    _locationDirectives["autoindex"]            = &LocationConfig::setAutoIndex;
    _locationDirectives["index"]                = &LocationConfig::setIndexes;
    _locationDirectives["client_max_body_size"] = &LocationConfig::setClientMaxBody;
    _locationDirectives["methods"]              = &LocationConfig::setAllowedMethods;
    _locationDirectives["return"]               = &LocationConfig::setRedirect;
    _locationDirectives["cgi_pass"]             = &LocationConfig::setCgiPass;
    _locationDirectives["upload_dir"]           = &LocationConfig::setUploadDir;
    _locationDirectives["error_page"]           = &LocationConfig::setErrorPage;
}

ConfigParser::~ConfigParser() {}

void ConfigParser::nextToken() {
    //_current is (type, value, line)
    _current = _lexer.nextToken();
}

bool ConfigParser::error(const String& msg) {
    Logger::error("Config error at line " + typeToString(_current.getLine()) + ": " + msg);
    return false;
}

bool ConfigParser::expect(Type type, const String& expectedDesc) {
    if (_current.getType() != type) {
        return error("Expected " + expectedDesc + ", got '" + _current.getValue() + "'");
    }
    nextToken();
    return true;
}

bool ConfigParser::parse() {
    try {
        while (_current.getType() != TOKEN_EOF) {
            if (_current.getType() == TOKEN_WORD && _current.getValue() == "http") {
                if (_haveHttp)
                    return error("Duplicate http block");
                if (!parseHttp())
                    return false;
                _haveHttp = true;
            } else if (_current.getType() == TOKEN_WORD && _current.getValue() == "server") {
                if (!parseServer())
                    return false;
            } else {
                return error("Unexpected top-level token '" + _current.getValue() + "'");
            }
        }
        return validate();
    } catch (const std::exception& e) {
        Logger::error(String("Parser exception: ") + e.what());
        return false;
    }
}

bool ConfigParser::parseHttp() {
    nextToken();
    if (!expect(TOKEN_LBRACE, "'{' after http"))
        return false;

    while (_current.getType() != TOKEN_RBRACE) {
        if (_current.getType() == TOKEN_WORD && _current.getValue() == "server") {
            if (!parseServer())
                return false;
        } else if (_current.getType() == TOKEN_WORD && _current.getValue() == "client_max_body_size") {
            nextToken();
            if (_current.getType() != TOKEN_WORD && _current.getType() != TOKEN_STRING)
                return error("Expected size value after client_max_body_size");
            if (_httpClientMaxBody != -1)
                return error("Duplicate client_max_body_size in http block");
            _httpClientMaxBody = convertMaxBodySize(_current.getValue());
            nextToken();
            if (!expect(TOKEN_SEMICOLON, "';' after client_max_body_size"))
                return false;
        } else {
            return error("Invalid directive in http block: '" + _current.getValue() + "'");
        }
    }
    nextToken();
    return true;
}

bool ConfigParser::parseServer() {
    nextToken(); // consume "server"
    if (!expect(TOKEN_LBRACE, "'{' after server"))
        return false;

    ServerConfig srv;
    while (_current.getType() != TOKEN_RBRACE) {
        if (_current.getType() == TOKEN_WORD && _current.getValue() == "location") {
            if (!parseLocation(srv))
                return false;
        } else {
            if (_current.getType() != TOKEN_WORD)
                return error("Expected directive name");
            String key = _current.getValue();
            nextToken();

            VectorString values;
            while (_current.getType() != TOKEN_SEMICOLON) {
                if (_current.getType() != TOKEN_WORD && _current.getType() != TOKEN_STRING)
                    return error("Expected value or ';'");
                values.push_back(_current.getValue());
                nextToken();
            }
            nextToken();

            if (!keyExists(_serverDirectives, key))
                return error("Unknown server directive '" + key + "'");
            const ServerSetter setter = getValue<ServerDirectiveMap, String, ServerSetter>(_serverDirectives, key);
            if (!(srv.*(setter))(values))
                return false;
        }
    }
    nextToken();
    _servers.push_back(srv);
    return true;
}

bool ConfigParser::parseLocation(ServerConfig& srv) {
    nextToken();
    if (_current.getType() != TOKEN_WORD && _current.getType() != TOKEN_STRING)
        return error("Expected location path");
    String path = _current.getValue();
    if (path.empty() || path[0] != '/')
        return error("Location path must start with '/'");

    nextToken();

    if (!expect(TOKEN_LBRACE, "'{' after location path"))
        return false;
    const VectorLocationConfig& existing = srv.getLocations();
    for (size_t i = 0; i < existing.size(); ++i) {
        if (existing[i].getPath() == path)
            return error("Duplicate location path: " + path);
    }

    LocationConfig loc(path);
    while (_current.getType() != TOKEN_RBRACE) {
        if (_current.getType() == TOKEN_WORD && _current.getValue() == "location") {
            return error("Nested locations are not supported");
        } else {
            if (_current.getType() != TOKEN_WORD)
                return error("Expected directive name");
            String key = _current.getValue();
            nextToken();

            VectorString values;
            while (_current.getType() != TOKEN_SEMICOLON) {
                if (_current.getType() != TOKEN_WORD && _current.getType() != TOKEN_STRING)
                    return error("Expected value or ';'");
                values.push_back(_current.getValue());
                nextToken();
            }
            nextToken();

            if (!keyExists(_locationDirectives, key))
                return error("Unknown location directive '" + key + "'");

            const LocationSetter setter = getValue<LocationDirectiveMap, String, LocationSetter>(_locationDirectives, key);
            if (!(loc.*(setter))(values))
                return false;
        }
    }
    nextToken();
    srv.addLocation(loc);
    return true;
}
bool ConfigParser::validate() {
    if (_servers.empty())
        return Logger::error("No server defined");

    if (_httpClientMaxBody == -1)
        _httpClientMaxBody = 1024 * 1024; // default 1M

    for (size_t i = 0; i < _servers.size(); ++i) {
        ServerConfig& srv = _servers[i];
        if (srv.getListenAddresses().empty())
            return Logger::error("Server missing listen directive");
        if (srv.getLocations().empty())
            return Logger::error("Server must have at least one location");
        if (srv.getClientMaxBody() == -1)
            srv.setClientMaxBody(_httpClientMaxBody);

        VectorLocationConfig& locs = srv.getLocations();
        for (size_t j = 0; j < locs.size(); ++j) {
            LocationConfig& loc = locs[j];
            if (loc.getRoot().empty() && !loc.getIsRedirect()) {
                if (srv.getRoot().empty())
                    return Logger::error("Location has no root and server has no root");
                loc.setRoot(srv.getRoot());
            }
            if (loc.getAllowedMethods().empty())
                loc.setAllowedMethods(VectorString(1, "GET"));
            if (loc.getClientMaxBody() == -1)
                loc.setClientMaxBody(srv.getClientMaxBody());
            if (loc.getIndexes().empty()) {
                if (srv.getIndexes().empty())
                    loc.setIndexes(VectorString(1, "index.html"));
                else
                    loc.setIndexes(srv.getIndexes());
            }
        }
    }
    return true;
}

const VectorServerConfig& ConfigParser::getServers() const {
    return _servers;
}

const ssize_t& ConfigParser::getHttpClientMaxBody() const {
    return _httpClientMaxBody;
}