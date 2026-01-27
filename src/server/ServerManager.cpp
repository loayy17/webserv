#include "ServerManager.hpp"

ServerManager::ServerManager(const std::vector<ServerConfig>& _configs) : running(false), serverConfigs(_configs) {}

ServerManager::~ServerManager() {
    shutdown();
}

bool ServerManager::initialize() {
    if (serverConfigs.empty())
        return Logger::error("[ERROR]: No server configurations provided");
    if (!initializeServers(serverConfigs) || servers.empty())
        return Logger::error("[ERROR]: Failed to initialize servers");
    Logger::info("[INFO]: All servers initialized successfully");
    running = true;
    return Logger::info("[INFO]: ServerManager initialized");
}

bool ServerManager::initializeServers(const std::vector<ServerConfig>& configs) {
    for (size_t i = 0; i < configs.size(); i++) {
        Server* server = new Server(configs[i]);
        if (!server->init()) {
            Logger::error("[ERROR]: Failed to start server on port " + typeToString(configs[i].getPort()));
            delete server;
            continue;
        }
        pollManager.addFd(server->getFd(), POLLIN);
        servers.push_back(server);
        std::string name = configs[i].getServerName().empty() ? "default" : configs[i].getServerName();
        Logger::info("[INFO]: Server '" + name + "' listening on port " + typeToString(configs[i].getPort()));
    }
    return !servers.empty();
}

bool ServerManager::run() {
    if (!running)
        return Logger::error("[ERROR]: Cannot run server manager");

    while (running) {
        int eventCount = pollManager.pollConnections(100);
        if (eventCount <= 0)
            continue;

        for (size_t i = 0; i < pollManager.size() && eventCount > 0; i++) {
            if (!pollManager.hasEvent(i, POLLIN))
                continue;

            int fd = pollManager.getFd(i);

            if (isServerSocket(fd)) {
                Server* server = findServerByFd(fd);
                if (server)
                    acceptNewConnection(server);
            } else if (clients.find(fd) != clients.end()) {
                handleClientData(fd);
            }
            eventCount--;
        }
    }
}

bool ServerManager::acceptNewConnection(Server* server) {
    int clientFd = server->acceptConnection();
    if (clientFd < 0)
        return Logger::error("[ERROR]: Failed to accept new connection");
    Client* client           = new Client(clientFd);
    clients[clientFd]        = client;
    clientToServer[clientFd] = server;
    pollManager.addFd(clientFd, POLLIN);
    return Logger::info("[INFO]: Connection accepted on port " + typeToString(server->getPort()));
}

void ServerManager::handleClientData(int clientFd) {
    Client* client = clients[clientFd];

    if (client->receiveData() <= 0) {
        closeClientConnection(clientFd);
        return;
    }
    Server* server = clientToServer[clientFd];
    if (server)
        processRequest(client, server);
    closeClientConnection(clientFd);
}

void ServerManager::processRequest(Client* client, Server* server) {
    std::string buffer = client->getBuffer();
    if (buffer.find("\r\n\r\n") == std::string::npos)
        return;

    HttpRequest request;
    if (!request.parse(buffer)) {
        Logger::error("[ERROR]: Failed to parse HTTP request");
        HttpResponse bad;
        bad.setStatus(400, "Bad Request");
        bad.addHeader("Content-Type", "text/plain");
        bad.addHeader("Connection", "close");
        std::string body = "Bad Request";
        bad.addHeader("Content-Length", typeToString(body.size()));
        bad.setBody(body);
        client->sendData(bad.httpToString());
        return;
    }
    std::cout << "[INFO]: Request: " + request.getUri() + " on port " + typeToString(server->getPort()) << std::endl;

    HttpResponse response;
    ServerConfig config = server->getConfig();
    Router       router(serverConfigs, request);
    router.processRequest();
    // ? continue tomorrow...
    //TODO:
    client->sendData(response.httpToString());
}

void ServerManager::closeClientConnection(int clientFd) {
    for (size_t i = 0; i < pollManager.size(); i++) {
        if (pollManager.getFd(i) == clientFd) {
            pollManager.removeFd(i);
            break;
        }
    }
    clients[clientFd]->closeConnection();
    delete clients[clientFd];
    clients.erase(clientFd);
    clientToServer.erase(clientFd);
}

Server* ServerManager::findServerByFd(int serverFd) const {
    for (size_t i = 0; i < servers.size(); i++) {
        if (servers[i]->getFd() == serverFd) {
            return servers[i];
        }
    }
    return NULL;
}

bool ServerManager::isServerSocket(int fd) const {
    for (size_t i = 0; i < servers.size(); i++)
        if (servers[i]->getFd() == fd)
            return true;
    return false;
}

void ServerManager::shutdown() {
    if (!running)
        return;

    std::cout << "[INFO]: Shutting down..." << std::endl;
    running = false;

    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        it->second->closeConnection();
        delete it->second;
    }
    clients.clear();
    clientToServer.clear();

    for (size_t i = 0; i < servers.size(); i++) {
        servers[i]->stop();
        delete servers[i];
    }
    servers.clear();
    std::cout << "[INFO]: Shutdown complete" << std::endl;
}

size_t ServerManager::getServerCount() const {
    return servers.size();
}

size_t ServerManager::getClientCount() const {
    return clients.size();
}
