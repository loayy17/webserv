#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <sys/types.h>
#include <unistd.h>
#include <ctime>
#include <iostream>
#include "../handlers/CgiProcess.hpp"
#include "../utils/Utils.hpp"
class Client {
   private:
    int        client_fd;
    String     storeReceiveData;
    String     storeSendData;
    time_t     lastActivity;
    CgiProcess _cgi;
    bool       _keepAlive;
    String     remoteAddress;
    int        _requestCount;

   public:
    Client(const Client&);
    Client& operator=(const Client&);
    Client(int fd);
    Client();
    ~Client();

    ssize_t       receiveData();
    ssize_t       sendData();
    void          setSendData(const String& data);
    void          setRemoteAddress(const String& address);
    void          clearStoreReceiveData();
    bool          isTimedOut(int timeout) const;
    void          closeConnection();
    void          removeReceivedData(size_t len);
    const String& getStoreReceiveData() const;
    const String& getStoreSendData() const;
    int           getFd() const;
    String        getRemoteAddress() const;

    CgiProcess&       getCgi();
    const CgiProcess& getCgi() const;
    void              setKeepAlive(bool keepAlive);
    bool              isKeepAlive() const;
    void              incrementRequestCount();
    int               getRequestCount() const;
};

#endif
