#include "ServerManager.hpp"

ServerManager::ServerManager()
    : epollManager(), servers(), serverConfigs(), clients(), clientToServer(), serverToConfigs(), mimeTypes(), sessionManager() {}

ServerManager::ServerManager(const VectorServerConfig& _configs)
    : epollManager(), servers(), serverConfigs(_configs), clients(), clientToServer(), serverToConfigs(), mimeTypes(), sessionManager() {}

ServerManager::~ServerManager() {
    shutdown();
}

bool ServerManager::initialize() {
    if (serverConfigs.empty())
        return Logger::error("No server configurations provided");
    if (!epollManager.init())
        return Logger::error("Failed to initialize epoll");
    if (!initializeServers(serverConfigs) || servers.empty())
        return Logger::error("Failed to initialize servers");
    g_running = 1;
    return Logger::info("[INFO]: ServerManager initialized");
}

ListenerToConfigsMap ServerManager::mapListenersToConfigs(const VectorServerConfig& serversConfigs) {
    ListenerToConfigsMap result;
    for (size_t i = 0; i < serversConfigs.size(); i++) {
        const VectorListenAddress& addresses = serversConfigs[i].getListenAddresses();
        for (size_t j = 0; j < addresses.size(); j++) {
            String key = addresses[j].getInterface() + ":" + typeToString<int>(addresses[j].getPort());
            result[key].push_back(serversConfigs[i]);
        }
    }
    return result;
}

Server* ServerManager::initializeServer(const ServerConfig& serverConfig, size_t listenIndex) {
    Server* server = NULL;
    try {
        server = new Server(serverConfig, listenIndex);
        if (!server->init())
            throw std::runtime_error("Server initialization failed");
    } catch (...) {
        Logger::error("Server init failed for listen " + serverConfig.getListenAddresses()[listenIndex].getListenAddress());
        if (server) {
            if (server->getFd() >= 0) close(server->getFd());
            delete server;
        }
        return NULL;
    }
    return server;
}

Server* ServerManager::createServerForListener(const String& listenerKey, const VectorServerConfig& serversConfigsForListener,
                                               EpollManager& epollMgr) {
    if (serversConfigsForListener.empty())
        return NULL;
    const ServerConfig& firstConfig = serversConfigsForListener[0];
    const VectorListenAddress& addresses   = firstConfig.getListenAddresses();
    size_t                     listenIndex = 0;
    for (size_t i = 0; i < addresses.size(); i++) {
        if (addresses[i].getListenAddress() == listenerKey) {
            listenIndex = i;
            break;
        }
    }
    Server* server = initializeServer(firstConfig, listenIndex);
    if (!server)
        return NULL;
    epollMgr.addFd(server->getFd(), EPOLLIN);
    return server;
}

bool ServerManager::initializeServers(const VectorServerConfig& serversConfigs) {
    ListenerToConfigsMap      listenerToConfigs = mapListenersToConfigs(serversConfigs);
    for (ListenerToConfigsMap::iterator it = listenerToConfigs.begin(); it != listenerToConfigs.end(); ++it) {
        Server* server = createServerForListener(it->first, it->second, epollManager);
        if (!server) continue;
        servers.push_back(server);
        serverToConfigs[server->getFd()] = it->second;
    }
    return !servers.empty();
}

bool ServerManager::run() {
    if (!g_running) return false;
    time_t lastSessionCleanup = getCurrentTime();
    while (g_running) {
        int eventCount = epollManager.wait(10);
        checkTimeouts(CLIENT_TIMEOUT);
        if (getDifferentTime(lastSessionCleanup, getCurrentTime()) > SESSION_CLEANUP_INTERVAL) {
            sessionManager.cleanupExpiredSessions(SESSION_TIMEOUT);
            lastSessionCleanup = getCurrentTime();
        }
        if (eventCount <= 0) continue;

        for (int i = 0; i < eventCount; i++) {
            int fd = epollManager.getFd(i);
            uint32_t events = epollManager.getEvents(i);
            if (fd < 0) continue;

            try {
                if (isCgiPipe(fd)) {
                    if (events & EPOLLOUT) handleCgiWrite(fd);
                    if (events & (EPOLLIN | EPOLLHUP | EPOLLERR)) handleCgiRead(fd);
                    continue;
                }
                if (events & (EPOLLIN | EPOLLPRI)) {
                    if (isServerSocket(fd)) {
                        Server* server = findServerByFd(fd);
                        if (server) acceptNewConnection(server);
                    } else {
                        MapIntClientPtr::iterator it = clients.find(fd);
                        if (it != clients.end()) {
                            handleClientRead(fd);
                        }
                    }
                }
                if (events & EPOLLOUT) {
                    MapIntClientPtr::iterator it = clients.find(fd);
                    if (it != clients.end()) handleClientWrite(fd);
                }
                if ((events & (EPOLLERR | EPOLLHUP)) && !(events & (EPOLLIN | EPOLLOUT))) {
                    MapIntClientPtr::iterator it = clients.find(fd);
                    if (it != clients.end()) closeClientConnection(fd);
                }
            } catch (const std::exception& e) {
                Logger::error("Exception on fd " + typeToString(fd) + ": " + e.what());
                if (!isServerSocket(fd) && !isCgiPipe(fd) && clients.count(fd)) closeClientConnection(fd);
            }
        }
    }
    return true;
}

bool ServerManager::acceptNewConnection(Server* server) {
    String remoteAddress;
    int clientFd = server->acceptConnection(remoteAddress);
    if (clientFd < 0) return false;
    Client* client = new Client(clientFd);
    client->setRemoteAddress(remoteAddress);
    clients[clientFd] = client;
    clientToServer[clientFd] = server;
    epollManager.addFd(clientFd, EPOLLIN);
    return true;
}

void ServerManager::handleClientRead(int clientFd) {
    Client* client = getValue(clients, clientFd, (Client*)NULL);
    if (!client || client->receiveData() <= 0) {
        closeClientConnection(clientFd);
        return;
    }
    Server* server = getValue(clientToServer, clientFd, (Server*)NULL);
    if (server) processRequest(client, server);
}

void ServerManager::handleClientWrite(int clientFd) {
    Client* client = getValue(clients, clientFd, (Client*)NULL);
    if (!client || client->sendData() < 0) {
        closeClientConnection(clientFd);
        return;
    }
    if (client->getStoreSendData().empty()) {
        if (client->isKeepAlive()) epollManager.addFd(clientFd, EPOLLIN);
        else closeClientConnection(clientFd);
    }
}

void ServerManager::checkTimeouts(int timeout) {
    std::vector<int> toClose;
    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getCgi().isActive()) {
            if (getDifferentTime(it->second->getCgi().getStartTime(), getCurrentTime()) > CGI_TIMEOUT) {
                cleanupClientCgi(it->second);
                it->second->setSendData(ResponseBuilder(mimeTypes).buildError(HTTP_GATEWAY_TIMEOUT, "CGI Timeout").toString());
                it->second->setHeadersParsed(false);
                it->second->getRequest().clear();
                epollManager.addFd(it->first, EPOLLIN | EPOLLOUT);
            }
        } else if (it->second->isTimedOut(timeout)) {
            toClose.push_back(it->first);
        }
    }
    for (size_t i = 0; i < toClose.size(); i++) closeClientConnection(toClose[i]);
}

void ServerManager::sendErrorResponse(Client* client, int statusCode, const String& message, bool closeConnection, size_t bytesToRemove) {
    HttpResponse response = ResponseBuilder(mimeTypes).buildError(statusCode, message);
    if (closeConnection) {
        response.addHeader("Connection", "close");
        client->setKeepAlive(false);
    }
    client->setSendData(response.toString());
    if (bytesToRemove > 0) client->removeReceivedData(bytesToRemove);
    else client->clearStoreReceiveData();
    client->setHeadersParsed(false);
    client->getRequest().clear();
    epollManager.addFd(client->getFd(), EPOLLIN | EPOLLOUT);
}

void ServerManager::processRequest(Client* client, Server* server) {
    while (true) {
        if (!client->isHeadersParsed()) {
            const String& buffer = client->getStoreReceiveData();
            if (buffer.empty()) break;

            if (buffer.size() > MAX_HEADER_SIZE) {
                sendErrorResponse(client, HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE, getHttpStatusMessage(HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE), true, 0);
                return;
            }

            if (buffer[0] == '\r' || buffer[0] == '\n') {
                size_t start = 0;
                while (start < buffer.size() && (buffer[start] == '\r' || buffer[start] == '\n')) start++;
                client->removeReceivedData(start);
                continue;
            }

            size_t headerEnd = buffer.find(DOUBLE_CRLF);
            size_t headerEndLen = 4;
            if (headerEnd == String::npos) {
                headerEnd = buffer.find("\n\n");
                headerEndLen = 2;
            }
            if (headerEnd == String::npos) break;

            if (!client->getRequest().parseHeaders(buffer.substr(0, headerEnd))) {
                sendErrorResponse(client, client->getRequest().getErrorCode(), "Bad Request", true, headerEnd + headerEndLen);
                return;
            }
            client->getRequest().setPort(server->getPort());

            bool keepAlive = (client->getRequest().getHttpVersion() == HTTP_VERSION_1_1);
            String conn = toLowerWords(client->getRequest().getHeader("connection"));
            if (conn == "close") keepAlive = false;
            else if (conn == "keep-alive") keepAlive = true;
            client->setKeepAlive(keepAlive);

            client->setHeadersParsed(true);
            client->removeReceivedData(headerEnd + headerEndLen);

            Router router(serverToConfigs[server->getFd()], client->getRequest());
            RouteResult res = router.processRequest();
            res.setRemoteAddress(client->getRemoteAddress());

            if (res.getHandlerType() == CGI) {
                ssize_t cl = client->getRequest().getContentLength();
                bool isChunked = (toLowerWords(client->getRequest().getHeader("transfer-encoding")).find("chunked") != String::npos);
                if (cl <= 0 && !isChunked) client->getCgi().setWriteDone(true);

                ResponseBuilder builder(mimeTypes);
                builder.build(res, &client->getCgi(), VectorInt());
                if (client->getCgi().isActive()) {
                    registerCgiPipes(client);
                    break;
                }
            }
        }

        if (client->isHeadersParsed()) {
            bool isChunked = (toLowerWords(client->getRequest().getHeader("transfer-encoding")).find("chunked") != String::npos);
            if (client->getCgi().isActive()) {
                if (isChunked) {
                    String decoded;
                    if (decodeChunkedBody(client->getStoreReceiveData(), decoded)) {
                        client->getCgi().appendBuffer(decoded);
                        client->getCgi().setWriteDone(true);
                        client->clearStoreReceiveData();
                        epollManager.addFd(client->getCgi().getWriteFd(), EPOLLOUT);
                    }
                } else {
                    ssize_t cl = client->getRequest().getContentLength();
                    size_t currentBodySize = client->getRequest().getBody().size();
                    size_t toWrite = std::min(client->getStoreReceiveData().size(), (size_t)(cl - currentBodySize));
                    if (toWrite > 0) {
                        String part = client->getStoreReceiveData().substr(0, toWrite);
                        client->getCgi().appendBuffer(part);
                        client->getRequest().parseBody(client->getRequest().getBody() + part);
                        client->removeReceivedData(toWrite);
                        epollManager.addFd(client->getCgi().getWriteFd(), EPOLLOUT);
                    }
                    if (client->getRequest().getBody().size() >= (size_t)cl) client->getCgi().setWriteDone(true);
                }
                break;
            } else {
                ssize_t cl = client->getRequest().getContentLength();
                if (!isChunked && client->getStoreReceiveData().size() >= (size_t)cl) {
                    if (cl > 0) client->getRequest().parseBody(client->getStoreReceiveData().substr(0, cl));
                    Router router(serverToConfigs[server->getFd()], client->getRequest());
                    RouteResult res = router.processRequest();
                    res.setRemoteAddress(client->getRemoteAddress());

                    if (res.getHandlerType() == CGI) {
                        bool isChunked2 = (toLowerWords(client->getRequest().getHeader("transfer-encoding")).find("chunked") != String::npos);
                        if (cl <= 0 && !isChunked2) client->getCgi().setWriteDone(true);
                        ResponseBuilder(mimeTypes).build(res, &client->getCgi(), VectorInt());
                        if (client->getCgi().isActive()) {
                            registerCgiPipes(client);
                            break;
                        }
                    } else {
                        HttpResponse response = ResponseBuilder(mimeTypes).build(res, &client->getCgi(), VectorInt());
                        if (!client->isKeepAlive()) response.addHeader("Connection", "close");
                        else response.addHeader("Connection", "keep-alive");
                        client->setSendData(response.toString());
                        if (cl > 0) client->removeReceivedData(cl);
                        client->setHeadersParsed(false);
                        client->getRequest().clear();
                        epollManager.addFd(client->getFd(), EPOLLIN | EPOLLOUT);
                        continue;
                    }
                } else if (isChunked) {
                    String decoded;
                    if (decodeChunkedBody(client->getStoreReceiveData(), decoded)) {
                        client->getRequest().parseBody(decoded);
                        Router router(serverToConfigs[server->getFd()], client->getRequest());
                        RouteResult res = router.processRequest();
                        res.setRemoteAddress(client->getRemoteAddress());

                        if (res.getHandlerType() == CGI) {
                            client->getCgi().appendBuffer(decoded);
                            client->getCgi().setWriteDone(true);
                            ResponseBuilder(mimeTypes).build(res, &client->getCgi(), VectorInt());
                            if (client->getCgi().isActive()) {
                                registerCgiPipes(client);
                                break;
                            }
                        } else {
                            HttpResponse response = ResponseBuilder(mimeTypes).build(res, &client->getCgi(), VectorInt());
                            if (!client->isKeepAlive()) response.addHeader("Connection", "close");
                            else response.addHeader("Connection", "keep-alive");
                            client->setSendData(response.toString());
                            client->clearStoreReceiveData();
                            client->setHeadersParsed(false);
                            client->getRequest().clear();
                            epollManager.addFd(client->getFd(), EPOLLIN | EPOLLOUT);
                            continue;
                        }
                    }
                }
            }
        }
        break;
    }
}

void ServerManager::closeClientConnection(int clientFd) {
    Client* c = getValue(clients, clientFd, (Client*)NULL);
    if (c) {
        if (c->getCgi().isActive()) cleanupClientCgi(c);
        c->closeConnection();
        delete c;
    }
    epollManager.removeFd(clientFd);
    clients.erase(clientFd);
    clientToServer.erase(clientFd);
}

bool ServerManager::isCgiPipe(int fd) const { return cgiPipeToClient.count(fd); }

void ServerManager::removeCgiPipe(int pipeFd) {
    epollManager.removeFd(pipeFd);
    cgiPipeToClient.erase(pipeFd);
}

void ServerManager::registerCgiPipes(Client* client) {
    CgiProcess& cgi = client->getCgi();
    if (!cgi.isWriteDone()) {
        epollManager.addFd(cgi.getWriteFd(), EPOLLOUT);
        cgiPipeToClient[cgi.getWriteFd()] = client->getFd();
    } else {
        if (cgi.getWriteFd() != -1) {
            close(cgi.getWriteFd());
            cgi.setWriteFd(-1);
        }
    }
    epollManager.addFd(cgi.getReadFd(), EPOLLIN);
    cgiPipeToClient[cgi.getReadFd()] = client->getFd();
}

void ServerManager::handleCgiWrite(int pipeFd) {
    Client* client = getValue(clients, getValue(cgiPipeToClient, pipeFd, -1), (Client*)NULL);
    if (!client || client->getCgi().writeBody(pipeFd)) {
        removeCgiPipe(pipeFd);
        if (client && client->getCgi().getWriteFd() != -1) {
            close(client->getCgi().getWriteFd());
            client->getCgi().setWriteFd(-1);
        }
    }
}

void ServerManager::handleCgiRead(int pipeFd) {
    Client* client = getValue(clients, getValue(cgiPipeToClient, pipeFd, -1), (Client*)NULL);
    if (!client || !client->getCgi().handleRead()) {
        removeCgiPipe(pipeFd);
        if (client) {
            if (client->getCgi().getWriteFd() != -1) {
                removeCgiPipe(client->getCgi().getWriteFd());
                close(client->getCgi().getWriteFd());
                client->getCgi().setWriteFd(-1);
            }
            client->setSendData(ResponseBuilder(mimeTypes).buildCgiResponse(client->getCgi()).toString());
            client->setHeadersParsed(false);
            client->getRequest().clear();
            epollManager.addFd(client->getFd(), EPOLLIN | EPOLLOUT);
        }
    }
}

void ServerManager::cleanupClientCgi(Client* client) {
    if (client->getCgi().getWriteFd() != -1) removeCgiPipe(client->getCgi().getWriteFd());
    if (client->getCgi().getReadFd() != -1) removeCgiPipe(client->getCgi().getReadFd());
    client->getCgi().cleanup();
}

Server* ServerManager::findServerByFd(int serverFd) const {
    for (size_t i = 0; i < servers.size(); i++) if (servers[i]->getFd() == serverFd) return servers[i];
    return NULL;
}

bool ServerManager::isServerSocket(int fd) const { return findServerByFd(fd) != NULL; }

void ServerManager::shutdown() {
    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getCgi().isActive()) cleanupClientCgi(it->second);
        delete it->second;
    }
    clients.clear();
    for (size_t i = 0; i < servers.size(); i++) delete servers[i];
    servers.clear();
}

size_t ServerManager::getServerCount() const { return servers.size(); }
size_t ServerManager::getClientCount() const { return clients.size(); }
