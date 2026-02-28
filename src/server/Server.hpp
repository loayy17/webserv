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
#include "../utils/Utils.hpp"

class Server {
   private:
    int          server_fd;
    bool         running;
    ServerConfig config;
    size_t       listenIndex;

    bool createSocket();
    bool configureSocket();
    bool bindSocket();
    bool startListening();

    Server(const Server&);
    Server& operator=(const Server&);

   public:
    Server();
    Server(ServerConfig config, size_t listenIdx = 0);
    ~Server();

    bool init();
    void stop();
    int  acceptConnection(String& remoteAddress);

    // getters
    int          getFd() const;
    int          getPort() const;
    bool         isRunning() const;
    const ServerConfig& getConfig() const;
};

#endif
