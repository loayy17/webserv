#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <sys/types.h>
#include <unistd.h>
#include <ctime>
#include <iostream>
#include "../handlers/CgiProcess.hpp"
#include "../http/HttpRequest.hpp"
#include "../utils/Utils.hpp"
class Client {
   private:
    int         client_fd;
    String      storeReceiveData;
    String      storeSendData;
    size_t      _sendOffset;
    time_t      lastActivity;
    CgiProcess  _cgi;
    bool        _keepAlive;
    String      remoteAddress;
    bool        _headersParsed;
    HttpRequest _request;

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
    bool          isHeadersParsed() const;
    void          setHeadersParsed(bool parsed);
    void          resetForNextRequest();
    HttpRequest&  getRequest();

    CgiProcess&       getCgi();
    const CgiProcess& getCgi() const;
    void              setKeepAlive(bool keepAlive);
    bool              isKeepAlive() const;
    bool              isChunkedEncoding() const;
    const String&     getMethod() const;
    size_t            getContentLength() const;
    int               getErrorCode() const;
    void              refreshActivity();
};

#endif
