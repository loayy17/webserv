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

    Client(const Client&);
    Client& operator=(const Client&);

   public:
    explicit Client(int fd);
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
