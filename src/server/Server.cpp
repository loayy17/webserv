#include "Server.hpp"
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include "../utils/Logger.hpp"

Server::Server() : server_fd(INVALID_FD), port(8080), running(false) {}

Server::Server(uint16_t port) : server_fd(INVALID_FD), port(port), running(false) {}
Server::~Server() {
    stop();
}

bool Server::createSocket() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        Logger::error("Failed to create socket");
        return false;
    }
    return true;
}
bool Server::configureSocket() {
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Logger::error("Failed to set SO_REUSEADDR");
        return false;
    }
    return true;
}
bool Server::bindSocket() {
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        Logger::error("Failed to bind socket");
        return false;
    }
    return true;
}
bool Server::startListening() {
    if (listen(server_fd, 10) < 0) {
        Logger::error("Failed to listen on socket");
        return false;
    }
    return true;
}

bool Server::init() {
    if (!createSocket() || !configureSocket() || !bindSocket() || !startListening()) {
        if (server_fd != INVALID_FD) {
            close(server_fd);
            server_fd = INVALID_FD;
        }
        return false;
    }
    running = true;
    Logger::info("Server started");
    return true;
}

void Server::stop() {
    if (server_fd != INVALID_FD) {
        close(server_fd);
        server_fd = INVALID_FD;
    }
    running = false;
}

bool Server::createNonBlockingSocket(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        Logger::error("Failed to get socket flags");
        return false;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        Logger::error("Failed to set non-blocking mode");
        return false;
    }
    return true;
}

int Server::acceptConnection(sockaddr_in* client_addr) {
    if (!running || server_fd == INVALID_FD) {
        Logger::error("Cannot accept connection: server not running");
        return INVALID_FD;
    }

    sockaddr_in  addr;
    socklen_t    addr_len  = sizeof(addr);
    sockaddr_in* addr_ptr  = client_addr ? client_addr : &addr;
    int          client_fd = accept(server_fd, (sockaddr*)addr_ptr, &addr_len);
    if (client_fd < 0) {
        // Don't log EAGAIN/EWOULDBLOCK - it's normal for non-blocking sockets
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            Logger::error("Failed to accept connection");
        }
        return INVALID_FD;
    }
    // Set client socket to non-blocking mode
    if (!createNonBlockingSocket(client_fd)) {
        close(client_fd);
        return INVALID_FD;
    }
    return client_fd;
}

int Server::getFd() const {
    return server_fd;
}