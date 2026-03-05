#include "Client.hpp"

Client::Client() : client_fd(-1), lastActivity(0), _keepAlive(false), _headersParsed(false) {}

Client::Client(const Client& other)
    : client_fd(other.client_fd),
      storeReceiveData(other.storeReceiveData),
      storeSendData(other.storeSendData),
      lastActivity(other.lastActivity),
      _cgi(other._cgi),
      _keepAlive(other._keepAlive),
      remoteAddress(other.remoteAddress),
      _headersParsed(other._headersParsed),
      _request(other._request) {}

Client& Client::operator=(const Client& other) {
    if (this != &other) {
        client_fd        = other.client_fd;
        storeReceiveData = other.storeReceiveData;
        storeSendData    = other.storeSendData;
        lastActivity     = other.lastActivity;
        _cgi             = other._cgi;
        _keepAlive       = other._keepAlive;
        remoteAddress    = other.remoteAddress;
        _headersParsed   = other._headersParsed;
        _request         = other._request;
    }
    return *this;
}

Client::Client(int fd) : client_fd(fd), _keepAlive(false), _headersParsed(false) {
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
        // if (storeReceiveData.size() > MAX_RECEIVE_BUFFER_SIZE)
        //     break;
    }
    if (total > 0) {
        updateTime(lastActivity);
        return total;
    }
    return n;
}

ssize_t Client::sendData() {
    if (storeSendData.empty())
        return 0;
    ssize_t sent = write(client_fd, storeSendData.c_str(), storeSendData.size());
    if (sent > 0) {
        storeSendData.erase(0, sent);
        updateTime(lastActivity);
    // } else if (sent < 0) {
    //     // Non-blocking write returned -1: could be EAGAIN (try later)
    //     // Since we cannot check errno, treat as transient - return 0 to keep connection alive
    //     // Real errors will be caught by POLLERR/POLLHUP in the next poll cycle
    //     return 0;
    }
    return sent;
}

void Client::setSendData(const String& data) {
    storeSendData = data;
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
    return getElapsedSeconds(lastActivity, getCurrentTime()) > timeout;
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

String Client::getRemoteAddress() const {
    return remoteAddress;
}

bool Client::isHeadersParsed() const {
    return _headersParsed;
}

void Client::setHeadersParsed(bool parsed) {
    _headersParsed = parsed;
}

void Client::resetForNextRequest() {
    _headersParsed = false;
    _request.clear();
}

HttpRequest& Client::getRequest() {
    return _request;
}

void Client::setKeepAlive(bool keepAlive) {
    _keepAlive = keepAlive;
}
bool Client::isKeepAlive() const {
    return _keepAlive;
}

void Client::refreshActivity() {
    updateTime(lastActivity);
}

bool Client::isChunkedEncoding() const {
    return toLowerWords(_request.getHeader("transfer-encoding")).find("chunked") != String::npos;
}

const String& Client::getMethod() const {
    return _request.getMethod();
}

size_t Client::getContentLength() const {
    return _request.getContentLength();
}

int Client::getErrorCode() const {
    return _request.getErrorCode();
}
