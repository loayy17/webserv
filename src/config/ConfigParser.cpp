#include "ConfigParser.hpp"
#include <cstdlib>
#include "../utils/Logger.hpp"

ConfigParser::ConfigParser() :
    file(""),
    servers(),
    scope(NONE),
    curr_index(0),
    httpClientMaxBody(""),
    lines(),
    serverDirectives(),
    locationDirectives()
{
}

ConfigParser::ConfigParser(const ConfigParser &other)
    : file(other.file),
      servers(other.servers),
      scope(other.scope),
      curr_index(other.curr_index),
      httpClientMaxBody(other.httpClientMaxBody),
      lines(other.lines),
      serverDirectives(other.serverDirectives),
      locationDirectives(other.locationDirectives)
{
}

ConfigParser &ConfigParser::operator=(const ConfigParser &other)
{
    if (this != &other)
    {
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

ConfigParser::ConfigParser(const std::string &f) : file(f), scope(NONE), curr_index(0)
{
    if (!convertFileToLines(file, lines))
        Logger::error("Failed to read configuration file: " + file);
}

ConfigParser::~ConfigParser() {
    servers.clear();
    lines.clear();
}

bool ConfigParser::getNextLine(std::string &out)
{
    if (curr_index >= lines.size())
        return false;
    out = lines[curr_index++];
    return true;
}

bool ConfigParser::parse()
{
    std::string line;
    bool is_http_defined = false;
    while (getNextLine(line))
    {
        if (line == "http {")
        {
            if (is_http_defined)
                return Logger::error("only one http block allowed");
            is_http_defined = true;
            if (scope != NONE)
                return Logger::error("http block in invalid position");
            scope = HTTP;
            if (!parseHttp())
                return false;
            scope = NONE;
        }
        else if (line == "server {")
        {
            if (scope != NONE)
                return Logger::error("server block in invalid position");
            scope = SERVER;
            if (!parseServer())
                return false;
            scope = NONE;
        }
        else
            return Logger::error("Invalid directive: " + line);
    }
    return validate();
}

bool ConfigParser::parseHttp()
{
    std::string line;
    while (getNextLine(line))
    {
        std::string t = trimSpacesComments(line);
        if (t.empty())
            continue;
        if (t == "}")
        {
            scope = NONE;
            break;
        }
        if (t == "server {")
        {
            if (!parseServer())
                return false;
            continue;
        }
        std::string key;
        std::vector<std::string> values;
        if (!parseKeyValue(t, key, values))
            return Logger::error("invalid http directive: " + t);

        if (key == "client_max_body_size")
        {
            if (!httpClientMaxBody.empty())
                return Logger::error("duplicate client_max_body_size");
            httpClientMaxBody = values[0];
        }
        else
            return Logger::error("Unknown http directive: " + key);
    }
    if (servers.size() == 0)
    {
        return Logger::error("No server defined");
    }
    if (scope != NONE)
        return Logger::error("Unexpected end of file, missing '}' in http block");
    return true;
}

ConfigParser::ServerDirectiveMap ConfigParser::getServerDirectives()
{
    static ConfigParser::ServerDirectiveMap m;

    m["listen"] = &ServerConfig::setListen;
    m["server_name"] = &ServerConfig::setServerName;
    m["root"] = &ServerConfig::setRoot;
    m["index"] = &ServerConfig::setIndexes;
    m["client_max_body_size"] = &ServerConfig::setClientMaxBody;

    return m;
}

bool ConfigParser::parseServer()
{
    ServerConfig srv;
    scope = SERVER;

    std::string l;
    serverDirectives = getServerDirectives();

    while (getNextLine(l))
    {
        std::string t = trimSpacesComments(l);
        if (t.empty())
            continue;
        if (t == "}")
        {
            scope = HTTP;
            break;
        }

        if (t.find("location ") == 0)
        {
            if (!parseLocation(srv, t))
                return false;
            continue;
        }

        if (!parseServerDirective(t, srv))
            return false;
    }
    if (srv.getPort() == -1 || srv.getInterface().empty())
        return Logger::error("server missing listen directive");
    if (srv.getLocations().empty())
        return Logger::error("at least one location is required");
    servers.push_back(srv);
    if (scope != HTTP)
        return Logger::error("Unexpected end of file, missing '}' in server block");
    return true;
}

bool ConfigParser::parseServerDirective(const std::string &l, ServerConfig &srv)
{
    std::string key;
    std::vector<std::string> values;
    if (!parseKeyValue(l, key, values))
        return Logger::error("invalid server directive: " + l);
    if (serverDirectives[key] == 0)
        return Logger::error("Unknown server directive: " + key);
    return (srv.*(serverDirectives[key]))(values);
}

bool ConfigParser::parseLocation(ServerConfig &srv, const std::string &header)
{
    std::string loc;
    std::vector<std::string> values;
    locationDirectives = getLocationDirectives();
    if (!parseKeyValue(header, loc, values))
        return Logger::error("invalid location syntax");
    if (values.size() != 2 || values[1] != "{")
        return Logger::error("Invalid location syntax");
    if (values.empty() || values[0][0] != '/')
        return Logger::error("Location path required");
    for (size_t i = 0; i < srv.getLocations().size(); i++)
    {
        if (srv.getLocations()[i].getPath() == values[0])
            return Logger::error("duplicate location path: " + values[0]);
    }

    LocationConfig locCfg(values[0]);
    scope = LOCATION;

    std::string line;
    while (getNextLine(line))
    {
        if (line == "}")
        {
            scope = SERVER;
            break;
        }
        else if (!parseLocationDirective(line, locCfg))
            return false;
    }
    srv.addLocation(locCfg);
    if (scope != SERVER)
        return Logger::error("Unexpected end of file, missing '}' in location block");
    return true;
}

ConfigParser::LocationDirectiveMap ConfigParser::getLocationDirectives()
{
    static ConfigParser::LocationDirectiveMap m;

    m["root"] = &LocationConfig::setRoot;
    m["autoindex"] = &LocationConfig::setAutoIndex;
    m["index"] = &LocationConfig::setIndexes;
    m["client_max_body_size"] = &LocationConfig::setClientMaxBody;
    m["methods"] = &LocationConfig::setAllowedMethods;
    m["return"] = &LocationConfig::setRedirect;
    m["cgi_path"] = &LocationConfig::setCgiPath;
    m["cgi_extension"] = &LocationConfig::setCgiExtension;
    m["upload_path"] = &LocationConfig::setUploadPath;

    return m;
}

bool ConfigParser::parseLocationDirective(const std::string &l, LocationConfig &loc)
{
    std::string key;
    std::vector<std::string> values;
    if (!parseKeyValue(l, key, values))
        return Logger::error("invalid location directive: " + l);
    if (locationDirectives[key] == 0)
        return Logger::error("Unknown location directive: " + key);
    return (loc.*(locationDirectives[key]))(values);
}

bool ConfigParser::validate()
{
    if (servers.empty())
        return Logger::error("No server defined");

    for (size_t i = 0; i < servers.size(); i++)
    {
        ServerConfig &s = servers[i];
        if (httpClientMaxBody.empty() && s.getClientMaxBody().empty())
        {
            httpClientMaxBody = "1M";
            s.setClientMaxBody("1M");
        }
        if (!httpClientMaxBody.empty() && s.getClientMaxBody().empty())
            s.setClientMaxBody(httpClientMaxBody);
        std::vector<LocationConfig> &locs = s.getLocations();
        for (size_t j = 0; j < locs.size(); j++)
        {
            if (locs[j].getRoot().empty())
            {
                if (s.getRoot().empty())
                    return Logger::error("location has no root and server has no root");
                locs[j].setRoot(s.getRoot());
            }
            if (locs[j].getAllowedMethods().empty())
                locs[j].addAllowedMethod("GET");
            if (locs[j].getClientMaxBody().empty())
                locs[j].setClientMaxBody(s.getClientMaxBody());
        }
    }
    return true;
}

std::vector<ServerConfig> ConfigParser::getServers() const
{
    return servers;
}

std::string ConfigParser::getHttpClientMaxBody() const
{
    return httpClientMaxBody;
}
