#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <unistd.h>
#include <ctime>
#include <string>

class Client {
   private:
    int         client_fd;
    std::string storeReceiveData;
    std::string storeSendData;
    time_t      lastActivity;

    public:
    Client(const Client&);
    Client& operator=(const Client&);
    Client(int fd);
    Client();
    ~Client();

    ssize_t     receiveData();
    ssize_t     sendData();
    void        queueResponse(const std::string& data);
    void        clearStoreReceiveData();
    bool        isTimedOut(int timeout) const;
    void        closeConnection();
    std::string getStoreReceiveData() const;
    std::string getStoreSendData() const;
    int         getFd() const;
};

#endif
