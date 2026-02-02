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
   public:
    ServerManager();
    ServerManager(const VectorServerConfig& configs);
    ServerManager(const ServerManager&);
    ServerManager& operator=(const ServerManager&);
    ~ServerManager();

    bool   initialize();
    bool   run();
    void   shutdown();
    size_t getServerCount() const;
    size_t getClientCount() const;

   private:
    bool                     running;
    PollManager              pollManager;
    std::vector<Server*>     servers;
    const VectorServerConfig serverConfigs;
    MapIntClientPtr          clients;
    MapIntServerPtr          clientToServer;
    MapIntVectorServerConfig serverToConfigs;
    // Internal helpers
    bool    initializeServers(const VectorServerConfig& serversConfigs);
    bool    acceptNewConnection(Server* server);
    void    handleClientRead(int clientFd);
    void    handleClientWrite(int clientFd);
    void    checkTimeouts(int timeout);
    void    closeClientConnection(int clientFd);
    Server* findServerByFd(int serverFd) const;
    bool    isServerSocket(int fd) const;
    void    processRequest(Client* client, Server* server);

    Server*              createServerForListener(const std::string& listenerKey, const VectorServerConfig& configs, PollManager& pollMgr);
    ListenerToConfigsMap getListerToConfigs();
    ListenerToConfigsMap mapListenersToConfigs(const VectorServerConfig& configs);
};

#endif
