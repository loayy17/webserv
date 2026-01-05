#include "Server.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include "../utils/Logger.hpp"

Server::Server() : server_fd(-1), port(8080), running(false) {}

Server::Server(uint16_t port) : server_fd(-1), port(port), running(false) {}

Server::~Server() {
    stop();
}

bool Server::init() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        return false;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0)
        return false;

    if (listen(server_fd, 1) < 0)
        return false;

    running = true;
    Logger::info("Server started");
    return true;
}

void Server::stop() {
    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }
    running = false;
}

int Server::acceptConnection() {
    sockaddr_in addr;
    socklen_t   len = sizeof(addr);
    return accept(server_fd, (sockaddr*)&addr, &len);
}

int Server::getFd() const {
    return server_fd;
}

bool Server::isRunning() const {
    return running;
}
