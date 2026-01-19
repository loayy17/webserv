#include "ServerManager.hpp"
#include <iostream>
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"

ServerManager::ServerManager() : running(false) {}

ServerManager::~ServerManager() {
    shutdown();
}

bool ServerManager::initialize(const std::vector<ServerConfig>& configs) {
    if (configs.empty()) {
        std::cout << "[ERROR]: No server configurations provided" << std::endl;
            return false;
    }

    if (!initializeServers(configs) || servers.empty()) {
        std::cout << "[ERROR]: Failed to initialize servers" << std::endl;
        return false;
    }

    std::cout << "[INFO]: Successfully initialized " + toString(servers.size()) + " server(s)" << std::endl;
    running = true;
    return true;
}

bool ServerManager::initializeServers(const std::vector<ServerConfig>& configs) {
    for (size_t i = 0; i < configs.size(); i++) {
        Server* server = new Server(configs[i]);
        
        if (!server->init()) {
            std::cout << "[ERROR]: Failed to start server on port " + toString(configs[i].getPort()) << std::endl;
            delete server;
            continue;
        }

        pollManager.addFd(server->getFd(), POLLIN);
        servers.push_back(server);
        
        std::string name = configs[i].getServerName().empty() ? "default" : configs[i].getServerName();
        std::cout << "[INFO]: Server '" + name + "' listening on port " + toString(configs[i].getPort()) << std::endl;
    }
    return !servers.empty();
}

void ServerManager::run() {
    if (!running) {
        std::cout << "[ERROR]: ServerManager not initialized" << std::endl;
        return;
    }

    std::cout << "[INFO]: Server manager started" << std::endl;

    while (running) {
        int eventCount = pollManager.pollConnections(100);
        if (eventCount <= 0) continue;

        for (size_t i = 0; i < pollManager.size() && eventCount > 0; i++) {
            if (!pollManager.hasEvent(i, POLLIN)) continue;

            int fd = pollManager.getFd(i);

            if (isServerSocket(fd)) {
                Server* server = findServerByFd(fd);
                if (server) acceptNewConnection(server);
            } else if (clients.find(fd) != clients.end()) {
                handleClientData(fd);
            }
            eventCount--;
        }
    }
}

void ServerManager::acceptNewConnection(Server* server) {
    int clientFd = server->acceptConnection();
    if (clientFd < 0) return;

    Client* client = new Client(clientFd);
    clients[clientFd] = client;
    clientToServer[clientFd] = server;
    pollManager.addFd(clientFd, POLLIN);

    std::cout << "[INFO]: Connection accepted on port " + toString(server->getPort()) << std::endl;
}

void ServerManager::handleClientData(int clientFd) {
    Client* client = clients[clientFd];
    
    if (client->receiveData() <= 0) {
        closeClientConnection(clientFd);
        return;
    }

    Server* server = clientToServer[clientFd];
    if (server) {
        processRequest(client, server);
    }
    
    closeClientConnection(clientFd);
}

void ServerManager::processRequest(Client* client, Server* server) {
    std::string buffer = client->getBuffer();
    if (buffer.find("\r\n\r\n") == std::string::npos) return;

    HttpRequest request;
    request.parseRequest(buffer);
    std::cout << "[INFO]: Request: " + request.getUri() + " on port " + toString(server->getPort()) << std::endl;

    HttpResponse response;
    ServerConfig config = server->getConfig();

    // Build HTML response
    std::string html = "<!DOCTYPE html><html><head><title>Webserv</title>";
    html += "<style>body{font-family:Arial;margin:40px;background:#f5f5f5;}";
    html += ".container{background:white;padding:30px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
    html += "h1{color:#2c3e50;border-bottom:3px solid #3498db;padding-bottom:10px;}";
    html += ".info{background:#ecf0f1;padding:15px;border-radius:5px;margin:10px 0;}";
    html += ".label{font-weight:bold;color:#2c3e50;} .value{color:#3498db;}</style></head><body>";
    html += "<div class='container'><h1>🚀 Welcome to " + 
            (config.getServerName().empty() ? "Webserv" : config.getServerName()) + "</h1>";
    html += "<p>Your HTTP server is running successfully!</p><div class='info'>";
    html += "<div><span class='label'>Port:</span> <span class='value'>" + toString(config.getPort()) + "</span></div>";
    
    if (!config.getServerName().empty()) {
        html += "<div><span class='label'>Server Name:</span> <span class='value'>" + config.getServerName() + "</span></div>";
    }
    if (!config.getRoot().empty()) {
        html += "<div><span class='label'>Root:</span> <span class='value'>" + config.getRoot() + "</span></div>";
    }
    
    html += "<div><span class='label'>Locations:</span> <span class='value'>" + toString(config.getLocations().size()) + "</span></div>";
    html += "</div></div></body></html>";

    response.setStatus(200, "OK");
    response.addHeader("Content-Type", "text/html; charset=utf-8");
    response.addHeader("Connection", "close");
    response.addHeader("Server", "Webserv/1.0");
    response.setBody(html);

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
    for (size_t i = 0; i < servers.size(); i++) {
        if (servers[i]->getFd() == fd) {
            return true;
        }
    }
    return false;
}

void ServerManager::shutdown() {
    if (!running) return;

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
