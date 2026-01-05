#include "Client.hpp"

Client::Client() : client_fd(-1) {}

Client::Client(int fd) : client_fd(fd) {}

Client::~Client() {
    closeConnection();
}

ssize_t Client::receiveData() {
    char    tmp[4096];
    ssize_t n = read(client_fd, tmp, sizeof(tmp));
    if (n > 0) {
        buffer.append(tmp, n);
    }
    return n;
}

ssize_t Client::sendData(const std::string& data) {
    return write(client_fd, data.c_str(), data.size());
}

std::string Client::getBuffer() const {
    return buffer;
}

void Client::clearBuffer() {
    buffer.clear();
}

void Client::closeConnection() {
    if (client_fd != -1) {
        close(client_fd);
        client_fd = -1;
    }
}

int Client::getFd() const {
    return client_fd;
}
