#include "Client.hpp"
#include <errno.h>
#include <unistd.h>

Client::Client(int fd) : client_fd(fd) {}

Client::~Client() {
    closeConnection();
}

ssize_t Client::receiveData() {
    char    tmp[1024];
    ssize_t total = 0;
    ssize_t n;
    
    while ((n = read(client_fd, tmp, sizeof(tmp))) > 0) {
        buffer.append(tmp, n);
        total += n;
    }
    
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return total > 0 ? total : 1;
    }
    return n;
}

ssize_t Client::sendData(const std::string& data) {
    return write(client_fd, data.c_str(), data.size());
}

std::string Client::getBuffer() const {
    return buffer;
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
