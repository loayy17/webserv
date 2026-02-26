#include "Client.hpp"

#define MAX_RECEIVE_PER_CALL (1024 * 1024) // 1MB max per poll iteration

Client::Client() : client_fd(-1), _sendOffset(0), lastActivity(0), _keepAlive(false), _requestCount(0) {}

Client::Client(const Client& other)
    : client_fd(other.client_fd),
      storeReceiveData(other.storeReceiveData),
      storeSendData(other.storeSendData),
      _sendOffset(other._sendOffset),
      lastActivity(other.lastActivity),
      _cgi(other._cgi),
      _keepAlive(other._keepAlive),
      remoteAddress(other.remoteAddress),
      _requestCount(other._requestCount) {}

Client& Client::operator=(const Client& other) {
    if (this != &other) {
        client_fd        = other.client_fd;
        storeReceiveData = other.storeReceiveData;
        storeSendData    = other.storeSendData;
        _sendOffset      = other._sendOffset;
        lastActivity     = other.lastActivity;
        _cgi             = other._cgi;
        _keepAlive       = other._keepAlive;
        remoteAddress    = other.remoteAddress;
        _requestCount    = other._requestCount;
    }
    return *this;
}

Client::Client(int fd) : client_fd(fd), _sendOffset(0), _keepAlive(false), _requestCount(0) {
    lastActivity = getCurrentTime();
}

Client::~Client() {
    closeConnection();
    if (_cgi.isActive()) {
        _cgi.cleanup();
    }
    storeReceiveData.clear();
    storeSendData.clear();
}

ssize_t Client::receiveData() {
    char    tmp[BUFFER_SIZE];
    ssize_t total = 0;
    ssize_t n;
    while ((n = read(client_fd, tmp, BUFFER_SIZE)) > 0) {
        storeReceiveData.append(tmp, n);
        total += n;
        if (total >= MAX_RECEIVE_PER_CALL)
            break;
    }
    if (total > 0)
        updateTime(lastActivity);
    return total > 0 ? total : n;
}

ssize_t Client::sendData() {
    if (_sendOffset >= storeSendData.size())
        return 0;
    ssize_t sent = write(client_fd, storeSendData.c_str() + _sendOffset, storeSendData.size() - _sendOffset);
    if (sent > 0) {
        _sendOffset += sent;
        updateTime(lastActivity);
        if (_sendOffset >= storeSendData.size()) {
            storeSendData.clear();
            _sendOffset = 0;
        }
    }
    return sent;
}

void Client::setSendData(const String& data) {
    storeSendData = data;
    _sendOffset   = 0;
}

void Client::setRemoteAddress(const String& address) {
    remoteAddress = address;
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

const String& Client::getRemoteAddress() const {
    return remoteAddress;
}

void Client::setKeepAlive(bool keepAlive) {
    _keepAlive = keepAlive;
}
bool Client::isKeepAlive() const {
    return _keepAlive;
}

void Client::incrementRequestCount() {
    _requestCount++;
}

int Client::getRequestCount() const {
    return _requestCount;
}
