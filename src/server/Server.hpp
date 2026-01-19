#ifndef SERVER_HPP
#define SERVER_HPP

#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <iostream>
#include "../config/ServerConfig.hpp"
class Server {
   private:
    int              server_fd;
    int              port;
    bool             running;
    ServerConfig     config;

    bool createSocket();
    bool configureSocket();
    bool bindSocket();
    bool startListening();
    bool createNonBlockingSocket(int fd);
    Server(const Server&);
    Server& operator=(const Server&);

   public:
    Server();
    Server(ServerConfig config);
    Server(int port);
    ~Server();

    bool init();
    void stop();
    int  acceptConnection(sockaddr_in* client_addr = 0);

    // getters
    int          getFd() const;
    int          getPort() const;
    bool         isRunning() const;
    ServerConfig getConfig() const;
};

#endif
