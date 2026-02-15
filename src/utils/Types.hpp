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
typedef std::string                          String;
typedef std::vector<String>                  VectorString;
typedef std::vector<int>                     VectorInt;
typedef std::map<String, String>             MapString;
typedef std::map<int, int>                   MapInt;
typedef std::map<String, VectorString>       MapValueVector;
typedef std::vector<ServerConfig>            VectorServerConfig;
typedef std::map<String, VectorServerConfig> ListenerToConfigsMap;
typedef std::vector<LocationConfig>          VectorLocationConfig;
typedef std::vector<ListenAddress>           VectorListenAddress;
typedef std::map<int, String>                MapIntString;
typedef std::map<int, Client*>               MapIntClientPtr;
typedef std::map<int, Server*>               MapIntServerPtr;
typedef std::map<int, VectorServerConfig>    MapIntVectorServerConfig;

typedef bool (ServerConfig::*ServerSetter)(const VectorString&);
typedef std::map<String, ServerSetter> ServerDirectiveMap;
typedef bool (LocationConfig::*LocationSetter)(const VectorString&);
typedef std::map<String, LocationSetter> LocationDirectiveMap;
#endif