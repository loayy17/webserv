#include "Client.hpp"
#include "../utils/Utils.hpp"

Client::Client() : client_fd(-1) {}

Client::Client(const Client& other) : client_fd(other.client_fd), storeReceiveData(other.storeReceiveData), storeSendData(other.storeSendData), lastActivity(other.lastActivity) {}

Client& Client::operator=(const Client& other) {
    if (this != &other) {
        client_fd = other.client_fd;
        lastActivity = other.lastActivity;
        storeReceiveData = other.storeReceiveData;
        storeSendData    = other.storeSendData;
    }
    return *this;
}

Client::Client(int fd) : client_fd(fd) {
    lastActivity = getCurrentTime();
}

Client::~Client() {
    closeConnection();
}

ssize_t Client::receiveData() {
    char    tmp[1024];
    ssize_t total = 0;
    ssize_t n;
    while ((n = read(client_fd, tmp, 1024)) > 0) {
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

void Client::queueResponse(const std::string& data) {
    storeSendData = data;
}

void Client::clearStoreReceiveData() {
    storeReceiveData.clear();
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

std::string Client::getStoreReceiveData() const {
    return storeReceiveData;
}

std::string Client::getStoreSendData() const {
    return storeSendData;
}

int Client::getFd() const {
    return client_fd;
}
