#include "Server.hpp"

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
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        return Logger::error("Failed to set SO_REUSEPORT");
    }
    return true;
}
bool Server::bindSocket() {
    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family        = AF_INET;
    hints.ai_socktype      = SOCK_STREAM;
    hints.ai_flags         = AI_PASSIVE;
    String      iface      = config.getInterface(listenIndex);
    int         portNum    = config.getPort(listenIndex);
    const char* interface  = iface.c_str();
    String      portString = typeToString<int>(portNum);
    const char* portStr    = portString.c_str();
    if (getaddrinfo(interface, portStr, &hints, &res) != 0)
        return Logger::error("getaddrinfo failed");
    int bindResult = bind(server_fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (bindResult < 0)
        return Logger::error("Failed to bind socket to " + iface + ":" + typeToString<int>(portNum));
    return Logger::info("Socket bound to http://" + iface + ":" + typeToString<int>(portNum));
}
bool Server::startListening() {
    if (listen(server_fd, SOMAXCONN) < 0)
        return Logger::error("Failed to listen on socket");
    return Logger::info("Server is listening on socket");
}

bool Server::init() {
    if (!createSocket() || !configureSocket() || !bindSocket() || !startListening()) {
        if (server_fd != -1) {
            close(server_fd);
            server_fd = -1;
        }
        return Logger::error("Server initialization failed");
    }
    if (!setNonBlocking(server_fd)) {
        close(server_fd);
        server_fd = -1;
        return Logger::error("Server initialization failed");
    }

    running = true;
    return Logger::info("Server initialized on port " + typeToString<int>(config.getPort(listenIndex)));
}

void Server::stop() {
    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }
    running = false;
}

int Server::acceptConnection(String& remoteAddress) {
    if (!running || server_fd == -1) {
        Logger::error("Cannot accept connection: server not running");
        return -1;
    }

    sockaddr_in addr;
    socklen_t   addr_len  = sizeof(addr);
    int         client_fd = accept(server_fd, (sockaddr*)&addr, &addr_len);
    if (client_fd < 0) {
        Logger::error("Failed to accept new connection");
        return -1;
    }
    if (!setNonBlocking(client_fd)) {
        close(client_fd);
        Logger::error("Failed to set non-blocking mode for client socket");
        return -1;
    }
    unsigned char* ip = (unsigned char*)&addr.sin_addr.s_addr;
    remoteAddress     = typeToString<int>(ip[0]) + "." + typeToString<int>(ip[1]) + "." + typeToString<int>(ip[2]) + "." + typeToString<int>(ip[3]);
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

const ServerConfig& Server::getConfig() const {
    return config;
}