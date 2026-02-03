// Lightweight typedefs used across the project
#ifndef TYPES_HPP
#define TYPES_HPP

#include <map>
#include <string>
#include <vector>

class ServerConfig;
class LocationConfig;
class ListenAddress;
class Client;
class Server;
typedef std::vector<std::string>                  VectorString;
typedef std::map<std::string, std::string>        MapString;
typedef std::map<std::string, VectorString>       MapValueVector;
typedef std::vector<ServerConfig>                 VectorServerConfig;
typedef std::map<std::string, VectorServerConfig> ListenerToConfigsMap;
typedef std::vector<LocationConfig>               VectorLocationConfig;
typedef std::vector<ListenAddress>                VectorListenAddress;
typedef std::map<int, std::string>                MapIntString;
typedef std::map<int, Client*>                    MapIntClientPtr;
typedef std::map<int, Server*>                    MapIntServerPtr;
typedef std::map<int, VectorServerConfig>         MapIntVectorServerConfig;


typedef bool (ServerConfig::*ServerSetter)(const VectorString&);
typedef std::map<std::string, ServerSetter> ServerDirectiveMap;
typedef bool (LocationConfig::*LocationSetter)(const VectorString&);
typedef std::map<std::string, LocationSetter>     LocationDirectiveMap;
#endif