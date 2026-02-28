#include "ServerManager.hpp"

ServerManager::ServerManager()
    : pollManager(), servers(), serverConfigs(), clients(), clientToServer(), serverToConfigs(), mimeTypes(), sessionManager() {}

ServerManager::ServerManager(const VectorServerConfig& _configs)
    : pollManager(), servers(), serverConfigs(_configs), clients(), clientToServer(), serverToConfigs(), mimeTypes(), sessionManager() {}

ServerManager::~ServerManager() {
    shutdown();
}

bool ServerManager::initialize() {
    if (serverConfigs.empty())
        return Logger::error("No server configurations provided");
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
            if (server->getFd() >= 0)
                close(server->getFd());
            delete server;
        }
        return NULL;
    }
    return server;
}

Server* ServerManager::createServerForListener(const String& listenerKey, const VectorServerConfig& serversConfigsForListener, PollManager& pollMgr) {
    if (serversConfigsForListener.empty())
        return NULL;
    const ServerConfig&        firstConfig = serversConfigsForListener[0];
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
    pollMgr.addFd(server->getFd(), POLLIN);
    return server;
}

bool ServerManager::initializeServers(const VectorServerConfig& serversConfigs) {
    ListenerToConfigsMap listenerToConfigs = mapListenersToConfigs(serversConfigs);
    for (ListenerToConfigsMap::iterator it = listenerToConfigs.begin(); it != listenerToConfigs.end(); ++it) {
        Server* server = createServerForListener(it->first, it->second, pollManager);
        if (!server)
            continue;
        servers.push_back(server);
        serverToConfigs[server->getFd()] = it->second;
    }
    return !servers.empty();
}

bool ServerManager::run() {
    if (!g_running)
        return false;
    time_t lastSessionCleanup = getCurrentTime();
    while (g_running) {
        int eventCount = pollManager.pollConnections(10);
        checkTimeouts(CLIENT_TIMEOUT);
        if (getDifferentTime(lastSessionCleanup, getCurrentTime()) > SESSION_CLEANUP_INTERVAL) {
            sessionManager.cleanupExpiredSessions(SESSION_TIMEOUT);
            lastSessionCleanup = getCurrentTime();
        }
        if (eventCount <= 0)
            continue;

        for (size_t i = 0; i < pollManager.size() && eventCount > 0; i++) {
            int fd = pollManager.getFd(i);
            if (fd < 0)
                continue;
            bool hasIn  = pollManager.hasEvent(i, POLLIN);
            bool hasOut = pollManager.hasEvent(i, POLLOUT);
            bool hasHup = pollManager.hasEvent(i, POLLHUP);
            bool hasErr = pollManager.hasEvent(i, POLLERR);

            if (!hasIn && !hasOut && !hasHup && !hasErr)
                continue;

            try {
                if (isCgiPipe(fd)) {
                    if (hasOut)
                        handleCgiWrite(fd);
                    if (hasIn || hasHup || hasErr)
                        handleCgiRead(fd);
                    eventCount--;
                    if (i < pollManager.size() && pollManager.getFd(i) != fd)
                        --i;
                    continue;
                }
                if (hasIn) {
                    if (isServerSocket(fd)) {
                        Server* server = findServerByFd(fd);
                        if (server)
                            acceptNewConnection(server);
                    } else if (clients.count(fd)) {
                        handleClientRead(fd);
                    }
                    eventCount--;
                }
                if (hasOut) {
                    if (clients.count(fd))
                        handleClientWrite(fd);
                    eventCount--;
                }
                if ((hasErr || hasHup) && !hasIn && !hasOut) {
                    if (clients.count(fd))
                        closeClientConnection(fd);
                    eventCount--;
                }
            } catch (const std::exception& e) {
                Logger::error("Exception on fd " + typeToString(fd) + ": " + e.what());
                if (!isServerSocket(fd) && !isCgiPipe(fd) && clients.count(fd))
                    closeClientConnection(fd);
            }
            if (i < pollManager.size() && pollManager.getFd(i) != fd)
                --i;
        }
    }
    return true;
}

bool ServerManager::acceptNewConnection(Server* server) {
    if (clients.size() >= MAX_CONNECTIONS) {
        Logger::error("Max connections reached (" + typeToString(MAX_CONNECTIONS) + "), rejecting new connection");
        String remoteAddress;
        int    tmpFd = server->acceptConnection(remoteAddress);
        if (tmpFd >= 0)
            close(tmpFd);
        return false;
    }
    String remoteAddress;
    int    clientFd = server->acceptConnection(remoteAddress);
    if (clientFd < 0)
        return false;
    Client* client = new Client(clientFd);
    client->setRemoteAddress(remoteAddress);
    clients[clientFd]        = client;
    clientToServer[clientFd] = server;
    pollManager.addFd(clientFd, POLLIN);
    return true;
}

void ServerManager::handleClientRead(int clientFd) {
    Client* client = getValue(clients, clientFd, (Client*)NULL);
    if (!client) {
        closeClientConnection(clientFd);
        return;
    }
    ssize_t received = client->receiveData();
    if (received == 0) {
        closeClientConnection(clientFd);
        return;
    }
    if (received < 0) {
        return;
    }
    Server* server = getValue(clientToServer, clientFd, (Server*)NULL);
    if (server)
        processRequest(client, server);
}

void ServerManager::handleClientWrite(int clientFd) {
    Client* client = getValue(clients, clientFd, (Client*)NULL);
    if (!client || client->sendData() < 0) {
        closeClientConnection(clientFd);
        return;
    }
    if (client->getStoreSendData().empty()) {
        if (client->isKeepAlive())
            pollManager.addFd(clientFd, POLLIN);
        else
            closeClientConnection(clientFd);
    }
}

void ServerManager::checkTimeouts(int timeout) {
    std::vector<int> toClose;
    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getCgi().isActive()) {
            if (getDifferentTime(it->second->getCgi().getStartTime(), getCurrentTime()) > CGI_TIMEOUT) {
                cleanupClientCgi(it->second);
                it->second->setSendData(responseBuilder.buildError(HTTP_GATEWAY_TIMEOUT, "CGI Timeout").toString());
                it->second->setHeadersParsed(false);
                it->second->getRequest().clear();
                pollManager.addFd(it->first, POLLIN | POLLOUT);
            }
        } else if (it->second->isTimedOut(timeout)) {
            toClose.push_back(it->first);
        }
    }
    for (size_t i = 0; i < toClose.size(); i++)
        closeClientConnection(toClose[i]);
}

void ServerManager::sendErrorResponse(Client* client, int statusCode, const String& message, bool closeConnection, size_t bytesToRemove) {
    HttpResponse response = responseBuilder.buildError(statusCode, message);
    if (closeConnection) {
        response.addHeader("Connection", "close");
        client->setKeepAlive(false);
    }
    client->setSendData(response.toString());
    if (bytesToRemove > 0)
        client->removeReceivedData(bytesToRemove);
    else
        client->clearStoreReceiveData();
    client->setHeadersParsed(false);
    client->getRequest().clear();
    pollManager.addFd(client->getFd(), POLLIN | POLLOUT);
}

void ServerManager::processRequest(Client* client, Server* server) {
    while (true) {
        if (!client->isHeadersParsed()) {
            if (!parseAndRouteHeaders(client, server))
                return;
            if (!client->isHeadersParsed())
                break;
        }

        if (client->isHeadersParsed()) {
            if (client->getCgi().isActive()) {
                handleCgiBodyStreaming(client);
                break;
            } else {
                if (handleRegularBody(client))
                    continue;
            }
        }
        break;
    }
}

ssize_t ServerManager::getMaxBodySize(const RouteResult& res) const {
    ssize_t maxBody = -1;
    if (res.getLocation())
        maxBody = res.getLocation()->getClientMaxBody();
    if (maxBody < 0 && res.getServer())
        maxBody = res.getServer()->getClientMaxBody();
    return maxBody;
}

void ServerManager::finalizeResponse(Client* client, const HttpResponse& response, ssize_t bodyLen) {
    HttpResponse resp = response;
    if (!client->isKeepAlive())
        resp.addHeader("Connection", "close");
    else
        resp.addHeader("Connection", "keep-alive");
    client->setSendData(resp.toString());
    if (bodyLen > 0)
        client->removeReceivedData(bodyLen);
    else
        client->clearStoreReceiveData();
    client->setHeadersParsed(false);
    client->getRequest().clear();
    clientRoutes.erase(client->getFd());
    pollManager.addFd(client->getFd(), POLLIN | POLLOUT);
}

bool ServerManager::parseAndRouteHeaders(Client* client, Server* server) {
    const String& buffer = client->getStoreReceiveData();
    if (buffer.empty())
        return false;

    if (buffer[0] == '\r' || buffer[0] == '\n') {
        size_t start = 0;
        while (start < buffer.size() && (buffer[start] == '\r' || buffer[start] == '\n'))
            start++;
        client->removeReceivedData(start);
        return true;
    }

    size_t headerEnd    = buffer.find(DOUBLE_CRLF);
    size_t headerEndLen = 4;
    if (headerEnd == String::npos) {
        headerEnd    = buffer.find("\n\n");
        headerEndLen = 2;
    }
    if (headerEnd == String::npos)
        return false;

    if (!client->getRequest().parseHeaders(buffer.substr(0, headerEnd))) {
        sendErrorResponse(client, client->getRequest().getErrorCode(), "Bad Request", true, headerEnd + headerEndLen);
        return false;
    }
    client->getRequest().setPort(server->getPort());

    bool   keepAlive = (client->getRequest().getHttpVersion() == HTTP_VERSION_1_1);
    String conn      = toLowerWords(client->getRequest().getHeader("connection"));
    if (conn == "close")
        keepAlive = false;
    else if (conn == "keep-alive")
        keepAlive = true;
    client->setKeepAlive(keepAlive);

    client->setHeadersParsed(true);
    client->removeReceivedData(headerEnd + headerEndLen);

    Router      router(serverToConfigs[server->getFd()], client->getRequest());
    RouteResult res = router.processRequest();
    res.setRemoteAddress(client->getRemoteAddress());
    clientRoutes[client->getFd()] = res;

    if (res.getStatusCode() >= 400) {
        sendErrorResponse(client, res.getStatusCode(),
                          res.getErrorMessage().empty() ? getHttpStatusMessage(res.getStatusCode()) : res.getErrorMessage(), true, 0);
        clientRoutes.erase(client->getFd());
        return false;
    }

    bool hasContentLength = !client->getRequest().getHeader("content-length").empty();
    bool isChunked        = (toLowerWords(client->getRequest().getHeader("transfer-encoding")).find("chunked") != String::npos);

    if (!validateRequestBody(client, res, hasContentLength, isChunked))
        return false;

    if (res.getHandlerType() == CGI) {
        ssize_t cl = client->getRequest().getContentLength();
        if (cl <= 0 && !isChunked)
            client->getCgi().setWriteDone(true);
        responseBuilder.build(res, &client->getCgi(), VectorInt());
        if (client->getCgi().isActive())
            registerCgiPipes(client);
    }
    return true;
}

bool ServerManager::validateRequestBody(Client* client, const RouteResult& res, bool hasContentLength, bool isChunked) {
    if (hasContentLength && isChunked) {
        sendErrorResponse(client, HTTP_BAD_REQUEST, getHttpStatusMessage(HTTP_BAD_REQUEST), true, 0);
        clientRoutes.erase(client->getFd());
        return false;
    }

    ssize_t maxBody = getMaxBodySize(res);
    String  method  = client->getRequest().getMethod();

    if ((method == "GET" || method == "DELETE" || method == "TRACE") && (hasContentLength || isChunked)) {
        sendErrorResponse(client, HTTP_BAD_REQUEST, getHttpStatusMessage(HTTP_BAD_REQUEST), true, 0);
        clientRoutes.erase(client->getFd());
        return false;
    }

    if ((method == "POST" || method == "PUT" || method == "PATCH") && !hasContentLength && !isChunked) {
        sendErrorResponse(client, HTTP_LENGTH_REQUIRED, getHttpStatusMessage(HTTP_LENGTH_REQUIRED), true, 0);
        clientRoutes.erase(client->getFd());
        return false;
    }

    if (hasContentLength) {
        size_t cl = client->getRequest().getContentLength();
        if (maxBody >= 0 && (ssize_t)cl > maxBody) {
            sendErrorResponse(client, HTTP_PAYLOAD_TOO_LARGE, getHttpStatusMessage(HTTP_PAYLOAD_TOO_LARGE), true, 0);
            clientRoutes.erase(client->getFd());
            return false;
        }
    }

    if (maxBody >= 0 && client->getStoreReceiveData().size() > (size_t)maxBody) {
        sendErrorResponse(client, HTTP_PAYLOAD_TOO_LARGE, getHttpStatusMessage(HTTP_PAYLOAD_TOO_LARGE), true, 0);
        clientRoutes.erase(client->getFd());
        return false;
    }
    return true;
}

void ServerManager::handleCgiBodyStreaming(Client* client) {
    bool        isChunked = (toLowerWords(client->getRequest().getHeader("transfer-encoding")).find("chunked") != String::npos);
    RouteResult boundRes  = getValue(clientRoutes, client->getFd(), RouteResult());
    ssize_t     maxBody   = getMaxBodySize(boundRes);

    if (isChunked) {
        if (maxBody >= 0 && client->getStoreReceiveData().size() > (size_t)maxBody) {
            sendErrorResponse(client, HTTP_PAYLOAD_TOO_LARGE, getHttpStatusMessage(HTTP_PAYLOAD_TOO_LARGE), true, 0);
            clientRoutes.erase(client->getFd());
            return;
        }
        String decoded;
        if (decodeChunkedBody(client->getStoreReceiveData(), decoded)) {
            if (maxBody >= 0 && decoded.size() > (size_t)maxBody) {
                sendErrorResponse(client, HTTP_PAYLOAD_TOO_LARGE, getHttpStatusMessage(HTTP_PAYLOAD_TOO_LARGE), true, 0);
                clientRoutes.erase(client->getFd());
                return;
            }
            client->getCgi().appendBuffer(decoded);
            client->getCgi().setWriteDone(true);
            client->clearStoreReceiveData();
            pollManager.addFd(client->getCgi().getWriteFd(), POLLOUT);
        }
    } else {
        size_t cl              = client->getRequest().getContentLength();
        size_t currentBodySize = client->getRequest().getBody().size();
        if (cl < currentBodySize) {
            sendErrorResponse(client, HTTP_BAD_REQUEST, getHttpStatusMessage(HTTP_BAD_REQUEST), true, 0);
            clientRoutes.erase(client->getFd());
            return;
        }
        size_t toWrite = std::min(client->getStoreReceiveData().size(), cl - currentBodySize);
        if (toWrite > 0) {
            String part = client->getStoreReceiveData().substr(0, toWrite);
            if (maxBody >= 0 && (ssize_t)(currentBodySize + part.size()) > maxBody) {
                sendErrorResponse(client, HTTP_PAYLOAD_TOO_LARGE, getHttpStatusMessage(HTTP_PAYLOAD_TOO_LARGE), true, 0);
                clientRoutes.erase(client->getFd());
                return;
            }
            client->getCgi().appendBuffer(part);
            client->getRequest().parseBody(client->getRequest().getBody() + part);
            client->removeReceivedData(toWrite);
            if (client->getCgi().getWriteFd() != -1)
                pollManager.addFd(client->getCgi().getWriteFd(), POLLOUT);
        }
        if (client->getRequest().getBody().size() >= cl)
            client->getCgi().setWriteDone(true);
    }
}

bool ServerManager::handleRegularBody(Client* client) {
    bool    isChunked = (toLowerWords(client->getRequest().getHeader("transfer-encoding")).find("chunked") != String::npos);
    ssize_t cl        = client->getRequest().getContentLength();

    if (!isChunked && client->getStoreReceiveData().size() >= (size_t)cl) {
        if (cl > 0)
            client->getRequest().parseBody(client->getStoreReceiveData().substr(0, cl));
        RouteResult res = getValue(clientRoutes, client->getFd(), RouteResult());

        if (res.getHandlerType() == CGI) {
            bool isChunked2 = (toLowerWords(client->getRequest().getHeader("transfer-encoding")).find("chunked") != String::npos);
            if (cl <= 0 && !isChunked2)
                client->getCgi().setWriteDone(true);
            responseBuilder.build(res, &client->getCgi(), VectorInt());
            if (client->getCgi().isActive()) {
                registerCgiPipes(client);
                return true;
            }
        } else {
            HttpResponse response = responseBuilder.build(res, &client->getCgi(), VectorInt());
            finalizeResponse(client, response, cl);
            return true;
        }
    } else if (isChunked) {
        String decoded;
        if (decodeChunkedBody(client->getStoreReceiveData(), decoded)) {
            RouteResult res     = getValue(clientRoutes, client->getFd(), RouteResult());
            ssize_t     maxBody = getMaxBodySize(res);

            if (maxBody >= 0 && decoded.size() > (size_t)maxBody) {
                sendErrorResponse(client, HTTP_PAYLOAD_TOO_LARGE, getHttpStatusMessage(HTTP_PAYLOAD_TOO_LARGE), true, 0);
                clientRoutes.erase(client->getFd());
                return false;
            }

            client->getRequest().parseBody(decoded);

            if (res.getHandlerType() == CGI) {
                client->getCgi().appendBuffer(decoded);
                client->getCgi().setWriteDone(true);
                responseBuilder.build(res, &client->getCgi(), VectorInt());
                if (client->getCgi().isActive()) {
                    registerCgiPipes(client);
                    return true;
                }
            } else {
                HttpResponse response = responseBuilder.build(res, &client->getCgi(), VectorInt());
                finalizeResponse(client, response, 0);
                return true;
            }
        }
    }
    return false;
}

void ServerManager::closeClientConnection(int clientFd) {
    Client* c = getValue(clients, clientFd, (Client*)NULL);
    if (c) {
        if (c->getCgi().isActive())
            cleanupClientCgi(c);
        c->closeConnection();
        delete c;
    }
    pollManager.removeFdByValue(clientFd);
    clients.erase(clientFd);
    clientToServer.erase(clientFd);
}

bool ServerManager::isCgiPipe(int fd) const {
    return cgiPipeToClient.count(fd);
}

void ServerManager::removeCgiPipe(int pipeFd) {
    pollManager.removeFdByValue(pipeFd);
    cgiPipeToClient.erase(pipeFd);
}

void ServerManager::registerCgiPipes(Client* client) {
    CgiProcess& cgi = client->getCgi();
    if (!cgi.isWriteDone()) {
        pollManager.addFd(cgi.getWriteFd(), POLLOUT);
        cgiPipeToClient[cgi.getWriteFd()] = client->getFd();
    } else {
        if (cgi.getWriteFd() != -1) {
            close(cgi.getWriteFd());
            cgi.setWriteFd(-1);
        }
    }
    pollManager.addFd(cgi.getReadFd(), POLLIN);
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
            if (client->getCgi().getReadFd() != -1) {
                close(client->getCgi().getReadFd());
                client->getCgi().setReadFd(-1);
            }
            client->setSendData(responseBuilder.buildCgiResponse(client->getCgi()).toString());
            client->setHeadersParsed(false);
            client->getRequest().clear();
            clientRoutes.erase(client->getFd());
            client->getCgi().finish();
            pollManager.addFd(client->getFd(), POLLIN | POLLOUT);
        }
    }
}

void ServerManager::cleanupClientCgi(Client* client) {
    if (client->getCgi().getWriteFd() != -1)
        removeCgiPipe(client->getCgi().getWriteFd());
    if (client->getCgi().getReadFd() != -1)
        removeCgiPipe(client->getCgi().getReadFd());
    client->getCgi().cleanup();
}

Server* ServerManager::findServerByFd(int serverFd) const {
    for (size_t i = 0; i < servers.size(); i++)
        if (servers[i]->getFd() == serverFd)
            return servers[i];
    return NULL;
}

bool ServerManager::isServerSocket(int fd) const {
    return findServerByFd(fd) != NULL;
}

void ServerManager::shutdown() {
    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getCgi().isActive())
            cleanupClientCgi(it->second);
        delete it->second;
    }
    clients.clear();
    for (size_t i = 0; i < servers.size(); i++)
        delete servers[i];
    servers.clear();
}

size_t ServerManager::getServerCount() const {
    return servers.size();
}
size_t ServerManager::getClientCount() const {
    return clients.size();
}
