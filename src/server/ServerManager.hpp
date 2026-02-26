#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <vector>
#include "../config/MimeTypes.hpp"
#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../http/ResponseBuilder.hpp"
#include "../http/Router.hpp"
#include "../utils/Logger.hpp"
#include "../utils/SessionManager.hpp"
#include "../utils/Utils.hpp"
#include "Client.hpp"
#include "PollManager.hpp"
#include "Server.hpp"

extern volatile sig_atomic_t g_running;

class ServerManager {
   public:
    ServerManager();
    ServerManager(const VectorServerConfig& configs);
    ~ServerManager();

    bool   initialize();
    bool   run();
    void   shutdown();
    size_t getServerCount() const;
    size_t getClientCount() const;

   private:
    ServerManager(const ServerManager&);
    ServerManager&           operator=(const ServerManager&);
    PollManager              pollManager;
    std::vector<Server*>     servers;
    const VectorServerConfig serverConfigs;
    MapIntClientPtr          clients;
    MapIntServerPtr          clientToServer;
    MapIntVectorServerConfig serverToConfigs;
    MimeTypes                mimeTypes;
    SessionManager           sessionManager;
    MapInt                   cgiPipeToClient;

    // Internal helpers
    bool    initializeServers(const VectorServerConfig& serversConfigs);
    bool    acceptNewConnection(Server* server);
    void    handleClientRead(int clientFd);
    void    handleClientWrite(int clientFd);
    void    checkTimeouts(int timeout);
    void    reapCgiProcesses();
    void    closeClientConnection(int clientFd);
    Server* findServerByFd(int serverFd) const;
    bool    isServerSocket(int fd) const;
    bool    isCgiPipe(int fd) const;
    void    processRequest(Client* client, Server* server);
    Server* initializeServer(const ServerConfig& serverConfig, size_t listenIndex);
    void    sendErrorResponse(Client* client, int statusCode, const String& message, bool closeConnection, size_t bytesToRemove);
    // CGI pipe helpers
    void registerCgiPipes(Client* client);
    void handleCgiRead(int pipeFd);
    void handleCgiWrite(int pipeFd);
    void cleanupClientCgi(Client* client);
    void removeCgiPipe(int pipeFd);

    Server*              createServerForListener(const String& listenerKey, const VectorServerConfig& configs, PollManager& pollMgr);
    ListenerToConfigsMap getListerToConfigs();
    ListenerToConfigsMap mapListenersToConfigs(const VectorServerConfig& serversConfigs);
};

#endif
