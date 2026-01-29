#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <unistd.h>
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
    static const int                CLIENT_TIMEOUT = 30;
    bool                            running;
    PollManager                     pollManager;
    std::vector<Server*>            servers;
    const std::vector<ServerConfig> serverConfigs;
    std::map<int, Client*>          clients;
    std::map<int, Server*>          clientToServer;

    bool    initializeServers(const std::vector<ServerConfig>& configs);
    bool    acceptNewConnection(Server* server);
    void    handleClientRead(int clientFd);
    void    handleClientWrite(int clientFd);
    void    checkTimeouts(int timeout);
    void    closeClientConnection(int clientFd);
    Server* findServerByFd(int serverFd) const;
    bool    isServerSocket(int fd) const;
    void    processRequest(Client* client, Server* server);

   public:
    ServerManager();    
    ServerManager(const std::vector<ServerConfig>& configs);
    ServerManager(const ServerManager&);
    ServerManager& operator=(const ServerManager&);
    ~ServerManager();    

    bool   initialize();
    bool   run();
    void   shutdown();
    size_t getServerCount() const;
    size_t getClientCount() const;
};

#endif
