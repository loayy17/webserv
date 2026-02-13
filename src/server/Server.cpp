#include "Server.hpp"

Server::Server(const Server& other) : server_fd(other.server_fd), running(other.running), config(other.config), listenIndex(other.listenIndex) {}

Server& Server::operator=(const Server& other) {
    if (this != &other) {
        server_fd   = other.server_fd;
        running     = other.running;
        config      = other.config;
        listenIndex = other.listenIndex;
    }
    return *this;
}

Server::Server(ServerConfig cfg, size_t listenIdx) : server_fd(-1), running(false), config(cfg), listenIndex(listenIdx) {}

Server::Server() : server_fd(-1), running(false), config(ServerConfig()), listenIndex(0) {}

Server::~Server() {
    stop();
}
bool Server::createSocket() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return Logger::error("Failed to create socket");
    }
    return true;
}
bool Server::configureSocket() {
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        return Logger::error("Failed to set SO_REUSEADDR");
    }
    return true;
}
bool Server::bindSocket() {
    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family       = AF_INET;
    hints.ai_socktype     = SOCK_STREAM;
    hints.ai_flags        = AI_PASSIVE;
    String      iface     = config.getInterface(listenIndex);
    int         portNum   = config.getPort(listenIndex);
    const char* interface = iface.c_str();
    const char* portStr   = typeToString<int>(portNum).c_str();
    if (getaddrinfo(interface, portStr, &hints, &res) != 0)
        return Logger::error("getaddrinfo failed");
    int bindResult = bind(server_fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (bindResult < 0)
        return Logger::error("Failed to bind socket to " + iface + ":" + typeToString<int>(portNum));
    return Logger::info("Socket bound to http://" + iface + ":" + typeToString<int>(portNum));
}
bool Server::startListening() {
    if (listen(server_fd, 10) < 0) {
        return Logger::error("[ERROR]: Failed to listen on socket");
    }
    return Logger::info("[INFO]: Server is listening on socket");
}

bool Server::init() {
    if (!createSocket() || !configureSocket() || !bindSocket() || !startListening()) {
        if (server_fd != -1) {
            close(server_fd);
            server_fd = -1;
        }
        return Logger::error("[ERROR]: Server initialization failed");
    }
    if (!createNonBlockingSocket(server_fd)) {
        close(server_fd);
        server_fd = -1;
        return Logger::error("[ERROR]: Server initialization failed");
    }

    running = true;
    return Logger::info("[INFO]: Server initialized on port " + typeToString<int>(config.getPort(listenIndex)));
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
        return Logger::error("[ERROR]: Failed to get socket flags");
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return Logger::error("[ERROR]: Failed to set non-blocking mode");
    }
    return true;
}

int Server::acceptConnection(sockaddr_in* client_addr) {
    if (!running || server_fd == -1) {
        return Logger::error("[ERROR]: Cannot accept connection: server not running");
        return -1;
    }

    sockaddr_in  addr;
    socklen_t    addr_len  = sizeof(addr);
    sockaddr_in* addr_ptr  = client_addr ? client_addr : &addr;
    int          client_fd = accept(server_fd, (sockaddr*)addr_ptr, &addr_len);
    if (client_fd < 0) {
        Logger::error("[ERROR]: Failed to accept new connection");
        return -1;
    }
    if (!createNonBlockingSocket(client_fd)) {
        close(client_fd);
        Logger::error("[ERROR]: Failed to set non-blocking mode for client socket");
        return -1;
    }
    return client_fd;
}

int Server::getFd() const {
    return server_fd;
}
int Server::getPort() const {
    return config.getPort(listenIndex);
}
bool Server::isRunning() const {
    return running;
}

ServerConfig Server::getConfig() const {
    return config;
}