#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <iostream>
#include <map>
#include <vector>
#include "../config/MimeTypes.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../http/Router.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"
#include "Client.hpp"
#include "PollManager.hpp"
#include "Server.hpp"

class ServerManager {
   private:
    bool                            running;
    PollManager                     pollManager;
    std::vector<Server*>            servers;
    const std::vector<ServerConfig> serverConfigs;
    std::map<int, Client*>          clients;
    std::map<int, Server*>          clientToServer;

    bool    initializeServers(const std::vector<ServerConfig>& configs);
    bool    acceptNewConnection(Server* server);
    void    handleClientData(int clientFd);
    void    closeClientConnection(int clientFd);
    Server* findServerByFd(int serverFd) const;
    bool    isServerSocket(int fd) const;
    void    processRequest(Client* client, Server* server);

    ServerManager(const ServerManager&);
    ServerManager& operator=(const ServerManager&);

   public:
    ServerManager(const std::vector<ServerConfig>& configs);
    ~ServerManager();

    bool   initialize();
    bool   run();
    void   shutdown();
    size_t getServerCount() const;
    size_t getClientCount() const;
};

#endif
