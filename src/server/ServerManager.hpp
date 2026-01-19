#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <map>
#include <vector>
#include "../config/ServerConfig.hpp"
#include "Client.hpp"
#include "PollManager.hpp"
#include "Server.hpp"

class ServerManager {
   private:
    bool                   running;
    PollManager            pollManager;
    std::vector<Server*>   servers;
    std::map<int, Client*> clients;
    std::map<int, Server*> clientToServer;

    bool    initializeServers(const std::vector<ServerConfig>& configs);
    void    acceptNewConnection(Server* server);
    void    handleClientData(int clientFd);
    void    closeClientConnection(int clientFd);
    Server* findServerByFd(int serverFd) const;
    bool    isServerSocket(int fd) const;
    void    processRequest(Client* client, Server* server);
    
    ServerManager(const ServerManager&);
    ServerManager& operator=(const ServerManager&);

   public:
    ServerManager();
    ~ServerManager();

    bool   initialize(const std::vector<ServerConfig>& configs);
    void   run();
    void   shutdown();
    size_t getServerCount() const;
    size_t getClientCount() const;
};

#endif
