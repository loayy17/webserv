#ifndef SERVER_HPP
#define SERVER_HPP

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include "../config/ServerConfig.hpp"

class Server {
   private:
    int          server_fd;
    int          port;
    bool         running;
    ServerConfig config;

    bool createSocket();
    bool configureSocket();
    bool bindSocket();
    bool startListening();
    bool createNonBlockingSocket(int fd);

   public:
    Server();
    Server(const Server&);
    Server& operator=(const Server&);
    Server(ServerConfig config);
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
