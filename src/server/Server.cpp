#include "Server.hpp"
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include "../utils/Logger.hpp"

Server::Server(ServerConfig cfg) : server_fd(-1), running(false), config(cfg) {}

Server::Server() : server_fd(-1), running(false), config(ServerConfig()) {}

Server::~Server() {
    stop();
}

bool Server::createSocket() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cout << "[ERROR]: Failed to create socket" << std::endl;
        return false;
    }
    return true;
}
bool Server::configureSocket() {
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cout << "[ERROR]: Failed to set SO_REUSEADDR" << std::endl;
        return false;
    }
    return true;
}
bool Server::bindSocket() {
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family       = AF_INET;
    const char* interface = config.getInterface() == "localhost" ? "127.0.0.1" : config.getInterface().c_str();
    inet_pton(AF_INET, interface, &addr.sin_addr);
    addr.sin_port = htons(config.getPort());
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cout << "[ERROR]: Failed to bind socket" << std::endl;
        return false;
    }
    return true;
}
bool Server::startListening() {
    if (listen(server_fd, 10) < 0) {
        std::cout << "[ERROR]: Failed to listen on socket" << std::endl;
        return false;
    }
    return true;
}

bool Server::init() {
    if (!createSocket() || !configureSocket() || !bindSocket() || !startListening()) {
        if (server_fd != -1) {
            close(server_fd);
            server_fd = -1;
        }
        return false;
    }
    running = true;
    return true;
}

void Server::stop() {
    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }
    running = false;
}

bool Server::createNonBlockingSocket(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cout << "[ERROR]: Failed to get socket flags" << std::endl;
        return false;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cout << "[ERROR]: Failed to set non-blocking mode" << std::endl;
        return false;
    }
    return true;
}

int Server::acceptConnection(sockaddr_in* client_addr) {
    if (!running || server_fd == -1) {
        std::cout << "[ERROR]: Cannot accept connection: server not running" << std::endl;
        return -1;
    }

    sockaddr_in  addr;
    socklen_t    addr_len  = sizeof(addr);
    sockaddr_in* addr_ptr  = client_addr ? client_addr : &addr;
    int          client_fd = accept(server_fd, (sockaddr*)addr_ptr, &addr_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cout << "[ERROR]: Failed to accept connection" << std::endl;
        }
        return -1;
    }
    if (!createNonBlockingSocket(client_fd)) {
        close(client_fd);
        return -1;
    }
    return client_fd;
}

int Server::getFd() const {
    return server_fd;
}
int Server::getPort() const {
    return config.getPort();
}
bool Server::isRunning() const {
    return running;
}

ServerConfig Server::getConfig() const {
    return config;
}