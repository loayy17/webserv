#include "ServerManager.hpp"

ServerManager::ServerManager()
    : running(false), pollManager(), servers(), serverConfigs(), clients(), clientToServer(), serverToConfigs(), mimeTypes() {}

ServerManager::ServerManager(const ServerManager& other)
    : running(other.running),
      pollManager(other.pollManager),
      servers(other.servers),
      serverConfigs(other.serverConfigs),
      clients(other.clients),
      clientToServer(other.clientToServer),
      serverToConfigs(other.serverToConfigs),
      mimeTypes(other.mimeTypes) {}

ServerManager& ServerManager::operator=(const ServerManager& other) {
    if (this != &other) {
        running         = other.running;
        pollManager     = other.pollManager;
        servers         = other.servers;
        clients         = other.clients;
        clientToServer  = other.clientToServer;
        serverToConfigs = other.serverToConfigs;
        mimeTypes       = other.mimeTypes;
    }
    return *this;
}

ServerManager::ServerManager(const VectorServerConfig& _configs)
    : running(false), pollManager(), servers(), serverConfigs(_configs), clients(), clientToServer(), serverToConfigs(), mimeTypes() {}

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

ListenerToConfigsMap ServerManager::mapListenersToConfigs(const VectorServerConfig& configs) {
    ListenerToConfigsMap result;
    for (size_t i = 0; i < configs.size(); i++) {
        const VectorListenAddress& addresses = configs[i].getListenAddresses();
        for (size_t j = 0; j < addresses.size(); j++) {
            String key = addresses[j].getInterface() + ":" + typeToString<int>(addresses[j].getPort());
            result[key].push_back(configs[i]);
        }
    }
    return result;
}

Server* ServerManager::createServerForListener(const String& listenerKey, const VectorServerConfig& configsForListener, PollManager& pollManager) {
    if (configsForListener.empty())
        return NULL;

    const ServerConfig& firstConfig = configsForListener[0];

    const VectorListenAddress& addresses   = firstConfig.getListenAddresses();
    size_t                     listenIndex = 0;
    for (size_t i = 0; i < addresses.size(); i++) {
        String key = addresses[i].getInterface() + ":" + typeToString<int>(addresses[i].getPort());
        if (key == listenerKey) {
            listenIndex = i;
            break;
        }
    }
    Server* server = NULL;
    try {
        server = new Server(firstConfig, listenIndex);
        if (!server->init())
            throw std::runtime_error("Server init failed");
    } catch (...) {
        Logger::error("[ERROR]: Failed to create server for listener " + listenerKey);
        if (server)
            delete server;
        return NULL;
    }

    pollManager.addFd(server->getFd(), POLLIN);
    return server;
}

bool ServerManager::initializeServers(const VectorServerConfig& configs) {
    ListenerToConfigsMap listenerToConfigs = mapListenersToConfigs(configs);

    std::map<String, Server*> listenerToServerMap;

    ListenerToConfigsMap::iterator it;
    for (it = listenerToConfigs.begin(); it != listenerToConfigs.end(); ++it) {
        const String&             listenerKey        = it->first;
        const VectorServerConfig& configsForListener = it->second;

        Server* server = createServerForListener(listenerKey, configsForListener, pollManager);
        if (!server)
            continue;

        servers.push_back(server);
        listenerToServerMap[listenerKey] = server;
        serverToConfigs[server->getFd()] = configsForListener;
    }

    return !servers.empty();
}

bool ServerManager::run() {
    if (!running)
        return Logger::error("[ERROR]: Cannot run server manager");

    while (running) {
        int eventCount = pollManager.pollConnections(100);
        checkTimeouts(CLIENT_TIMEOUT);
        if (eventCount <= 0)
            continue;

        for (size_t i = 0; i < pollManager.size() && eventCount > 0; i++) {
            int fd = pollManager.getFd(i);

            // Handle read events
            if (pollManager.hasEvent(i, POLLIN)) {
                if (isServerSocket(fd)) {
                    Server* server = findServerByFd(fd);
                    if (server)
                        acceptNewConnection(server);
                } else if (clients.find(fd) != clients.end()) {
                    handleClientRead(fd);
                }
                eventCount--;
            }
            // Handle write events
            if (pollManager.hasEvent(i, POLLOUT)) {
                if (clients.find(fd) != clients.end()) {
                    handleClientWrite(fd);
                }
                eventCount--;
            }
        }
    }
    return true;
}

bool ServerManager::acceptNewConnection(Server* server) {
    int clientFd = server->acceptConnection();
    if (clientFd < 0)
        return Logger::error("[ERROR]: Failed to accept new connection");

    Client* client = NULL;
    try {
        client = new Client(clientFd);
    } catch (const std::bad_alloc& e) {
        Logger::error("[ERROR]: Memory allocation failed for client");
        close(clientFd);
        return false;
    }
    clients[clientFd]        = client;
    clientToServer[clientFd] = server;
    pollManager.addFd(clientFd, POLLIN | POLLOUT);
    return Logger::info("[INFO]: Connection accepted on port " + typeToString(server->getPort()));
}

void ServerManager::handleClientRead(int clientFd) {
    Client* client = getValue(clients, clientFd, (Client*)NULL);
    if (client == NULL) {
        closeClientConnection(clientFd);
        return;
    }

    if (client->receiveData() <= 0) {
        closeClientConnection(clientFd);
        return;
    }
    Logger::info("[INFO]: Data received from client");
    Server* server = getValue(clientToServer, clientFd, (Server*)NULL);
    if (server)
        processRequest(client, server);
}

void ServerManager::handleClientWrite(int clientFd) {
    Client* client = getValue(clients, clientFd, (Client*)NULL);
    if (client == NULL)
        return;

    if (client->getStoreSendData().empty())
        return;

    ssize_t sent = client->sendData();
    if (sent < 0) {
        closeClientConnection(clientFd);
        return;
    }

    // If all data sent, close connection
    if (client->getStoreSendData().empty()) {
        closeClientConnection(clientFd);
    }
}

void ServerManager::checkTimeouts(int timeout) {
    std::vector<int> toClose;

    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->isTimedOut(timeout)) {
            toClose.push_back(it->first);
        }
    }

    for (size_t i = 0; i < toClose.size(); i++) {
        Logger::info("[INFO]: Client timeout, closing connection");
        closeClientConnection(toClose[i]);
    }
}

void ServerManager::processRequest(Client* client, Server* /*server*/) {
    String buffer = client->getStoreReceiveData();
    if (buffer.find("\r\n\r\n") == String::npos) {
        Logger::info("[INFO]: Incomplete HTTP request, waiting for more data");
        return;
    }
    std::cout << "BUFFER" << buffer << std::endl;
    HttpRequest request;
    if (!request.parse(buffer)) {
        Logger::error("[ERROR]: Failed to parse HTTP request");
        ResponseBuilder builder;
        HttpResponse    response = builder.buildError(400, "Bad Request");
        client->setSendData(response.toString());
        client->clearStoreReceiveData();
        return;
    }
    
    Router router(serverConfigs, request);
    router.processRequest();
    ResponseBuilder builder(mimeTypes);
    HttpResponse    response = builder.build(router);
    client->setSendData(response.toString());
    client->clearStoreReceiveData();
}

void ServerManager::closeClientConnection(int clientFd) {
    for (size_t i = 0; i < pollManager.size(); i++) {
        if (pollManager.getFd(i) == clientFd) {
            pollManager.removeFd(i);
            break;
        }
    }
    Client* c = getValue(clients, clientFd, (Client*)NULL);
    if (c) {
        c->closeConnection();
        delete c;
    }
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

    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
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
