#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <unistd.h>
#include <ctime>
#include <iostream>
#include "../utils/Types.hpp"
class Client {
   private:
    int    client_fd;
    String storeReceiveData;
    String storeSendData;
    time_t lastActivity;

   public:
    Client(const Client&);
    Client& operator=(const Client&);
    Client(int fd);
    Client();
    ~Client();

    ssize_t receiveData();
    ssize_t sendData();
    void    setSendData(const String& data);
    void    clearStoreReceiveData();
    bool    isTimedOut(int timeout) const;
    void    closeConnection();
    String  getStoreReceiveData() const;
    String  getStoreSendData() const;
    int     getFd() const;
};

#endif
