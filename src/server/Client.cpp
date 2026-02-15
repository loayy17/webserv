#include "Client.hpp"

Client::Client() : client_fd(-1), lastActivity(0), _keepAlive(false) {}

Client::Client(const Client& other)
    : client_fd(other.client_fd),
      storeReceiveData(other.storeReceiveData),
      storeSendData(other.storeSendData),
      lastActivity(other.lastActivity),
      _cgi(other._cgi),
      _keepAlive(other._keepAlive) {}

Client& Client::operator=(const Client& other) {
    if (this != &other) {
        client_fd        = other.client_fd;
        storeReceiveData = other.storeReceiveData;
        storeSendData    = other.storeSendData;
        lastActivity     = other.lastActivity;
        _cgi             = other._cgi;
        _keepAlive       = other._keepAlive;
    }
    return *this;
}

Client::Client(int fd) : client_fd(fd), _keepAlive(false) {
    lastActivity = getCurrentTime();
}

Client::~Client() {
    closeConnection();
}

ssize_t Client::receiveData() {
    char    tmp[BUFFER_SIZE];
    ssize_t total = 0;
    ssize_t n;
    while ((n = read(client_fd, tmp, BUFFER_SIZE)) > 0) {
        storeReceiveData.append(tmp, n);
        total += n;
    }
    if (total > 0)
        updateTime(lastActivity);
    return total > 0 ? total : n;
}

ssize_t Client::sendData() {
    if (storeSendData.empty())
        return 0;
    ssize_t sent = write(client_fd, storeSendData.c_str(), storeSendData.size());
    if (sent > 0) {
        storeSendData.erase(0, sent);
        updateTime(lastActivity);
    }
    return sent;
}

void Client::setSendData(const String& data) {
    storeSendData = data;
}

void Client::clearStoreReceiveData() {
    storeReceiveData.clear();
}

void Client::removeReceivedData(size_t len) {
    if (len >= storeReceiveData.size())
        storeReceiveData.clear();
    else
        storeReceiveData.erase(0, len);
}

bool Client::isTimedOut(int timeout) const {
    return getDifferentTime(lastActivity, getCurrentTime()) > timeout;
}

void Client::closeConnection() {
    if (client_fd != -1) {
        close(client_fd);
        client_fd = -1;
    }
}

const String& Client::getStoreReceiveData() const {
    return storeReceiveData;
}
const String& Client::getStoreSendData() const {
    return storeSendData;
}
int Client::getFd() const {
    return client_fd;
}

CgiProcess& Client::getCgi() {
    return _cgi;
}
const CgiProcess& Client::getCgi() const {
    return _cgi;
}

void Client::setKeepAlive(bool keepAlive) {
    _keepAlive = keepAlive;
}
bool Client::isKeepAlive() const {
    return _keepAlive;
}
