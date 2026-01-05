#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>
#include <stdint.h>

class Server {
   private:
    int      server_fd;
    uint16_t port;
    bool     running;

    Server(const Server&);
    Server& operator=(const Server&);

   public:
    Server();
    Server(uint16_t port);
    ~Server();

    bool init();
    void stop();
    int  acceptConnection();
    int  getFd() const;
    bool isRunning() const;
};

#endif
