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
        serverFdMap[server->getFd()]     = server;
    }
    return !servers.empty();
}

bool ServerManager::run() {
    if (!g_running)
        return false;
    time_t lastSessionCleanup = getCurrentTime();
    while (g_running) {
        int eventCount = pollManager.pollConnections(POLL_TIMEOUT_MS);
        checkTimeouts(CLIENT_TIMEOUT);
        if (getElapsedSeconds(lastSessionCleanup, getCurrentTime()) > SESSION_CLEANUP_INTERVAL) {
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
                if (hasIn) {
                    if (isCgiPipe(fd))
                        handleCgiRead(fd);
                    else if (isServerSocket(fd)) {
                        Server* server = findServerByFd(fd);
                        if (server)
                            acceptNewConnection(server);
                    } else if (clients.count(fd))
                        handleClientRead(fd);
                }
                if (hasOut) {
                    if (isCgiPipe(fd))
                        handleCgiWrite(fd);
                    else if (clients.count(fd))
                        handleClientWrite(fd);
                }
                if ((hasErr || hasHup) && !hasIn && !hasOut) {
                    if (isCgiPipe(fd))
                        handleCgiRead(fd);
                    else if (clients.count(fd))
                        closeClientConnection(fd);
                }
                eventCount--;
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
        // i must accept the connection and immediately close it to prevent the client from hanging while trying to connect
        // in keep in kernel's accept queue until it times out, since the client will not receive any response until the connection is accepted and closed
        int tmpFd = server->acceptConnection(remoteAddress);
        if (tmpFd >= 0)
            close(tmpFd);
        return false;
    }
    String remoteAddress;
    int    clientFd = server->acceptConnection(remoteAddress);
    if (clientFd < 0)
        return false;
    Client* client;
    try {
        client = new Client(clientFd);
    } catch (...) {
        close(clientFd);
        return false;
    }
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
    if (received < 0)
        return;
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
            if (getElapsedSeconds(it->second->getCgi().getStartTime(), getCurrentTime()) > CGI_TIMEOUT) {
                cleanupClientCgi(it->second);
                it->second->setSendData(responseBuilder.buildError(HTTP_GATEWAY_TIMEOUT, "CGI Timeout").toString());
                it->second->resetForNextRequest();
                clientRoutes.erase(it->first);
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
        //     client->clearStoreReceiveData();
        // } else {
        //     response.addHeader("Connection", "keep-alive");
        //     if (bytesToRemove > 0)
        //         client->removeReceivedData(bytesToRemove);
    }
    client->setSendData(response.toString());
    //
    if (bytesToRemove > 0)
        client->removeReceivedData(bytesToRemove);
    else
        client->clearStoreReceiveData();
    //
    client->resetForNextRequest();
    clientRoutes.erase(client->getFd());
    pollManager.addFd(client->getFd(), POLLIN | POLLOUT);
}

void ServerManager::processRequest(Client* client, Server* server) {
    // while loop to handle cases where we receive the full request including body in one go, so we can process it immediately without waiting for another read event
    while (true) {
        if (!client->isHeadersParsed())
            if (!parseAndRouteHeaders(client, server))
                return;
        //     if (!client->isHeadersParsed())
        //         break;
        // }

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
    // if (bodyLen > 0)
    //     client->removeReceivedData(bodyLen);
    // else
    //     client->clearStoreReceiveData();
    client->removeReceivedData(bodyLen);
    client->resetForNextRequest();
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
        sendErrorResponse(client, client->getErrorCode(), getHttpStatusMessage(client->getErrorCode()), true, headerEnd + headerEndLen);
        return false;
    }
    client->getRequest().setPort(server->getPort());
    bool   keepAlive = (client->getRequest().getHttpVersion() == HTTP_VERSION_1_1);
    String conn      = toLowerWords(client->getRequest().getHeader(HEADER_CONNECTION));

    VectorString connValues;
    if (!splitByString(conn, connValues, ",")) {
        sendErrorResponse(client, HTTP_BAD_REQUEST, getHttpStatusMessage(HTTP_BAD_REQUEST), true, headerEnd + headerEndLen);
        return false;
    }
    for (size_t i = 0; i < connValues.size(); i++) {
        String val = trimSpaces(connValues[i]);
        if (val == CLOSE) {
            keepAlive = false;
            break;
        } else if (val == KEEP_ALIVE)
            keepAlive = true;
    }
    client->setKeepAlive(keepAlive);
    client->setHeadersParsed(true);
    client->removeReceivedData(headerEnd + headerEndLen);

    Router      router(serverToConfigs[server->getFd()], client->getRequest());
    RouteResult res = router.processRequest();
    res.setRemoteAddress(client->getRemoteAddress());
    clientRoutes[client->getFd()] = res;

    if (res.getStatusCode() >= 400) {
        // Try to consume body data from the buffer to preserve keep-alive
        // bool hasBody = !client->getRequest().getHeader(HEADER_CONTENT_LENGTH).empty() &&
        //                client->getContentLength() > 0;
        // bool isChunkedReq = client->isChunkedEncoding();
        // bool shouldClose = false;
        // size_t bodyBytesToRemove = 0;

        // if (hasBody) {
        //     size_t cl = client->getContentLength();
        //     if (client->getStoreReceiveData().size() >= cl)
        //         bodyBytesToRemove = cl;
        //     else
        //         shouldClose = true;
        // } else if (isChunkedReq) {
        //     size_t chunkedEnd = findChunkedBodyEnd(client->getStoreReceiveData());
        //     if (chunkedEnd != String::npos)
        //         bodyBytesToRemove = chunkedEnd;
        //     else
        //         shouldClose = true;
        // }

        sendErrorResponse(client, res.getStatusCode(),
                          res.getErrorMessage().empty() ? getHttpStatusMessage(res.getStatusCode()) : res.getErrorMessage(), true, 0);
        //                          res.getErrorMessage().empty() ? getHttpStatusMessage(res.getStatusCode()) : res.getErrorMessage(), shouldClose, bodyBytesToRemove);

        return false;
    }

    bool hasContentLength = !client->getRequest().getHeader(HEADER_CONTENT_LENGTH).empty();
    bool isChunked        = client->isChunkedEncoding();

    if (!validateRequestBody(client, res, hasContentLength, isChunked))
        return false;

    if (res.getHandlerType() == CGI) {
        ssize_t cl = client->getContentLength();
        if (cl <= 0 && !isChunked)
            client->getCgi().setWriteDone(true);
        responseBuilder.build(res, &client->getCgi(), getServerFds());
        if (client->getCgi().isActive())
            registerCgiPipes(client);
    }
    return true;
}

bool ServerManager::validateRequestBody(Client* client, const RouteResult& res, bool hasContentLength, bool isChunked) {
    if (hasContentLength && isChunked) {
        sendErrorResponse(client, HTTP_BAD_REQUEST, getHttpStatusMessage(HTTP_BAD_REQUEST), true, 0);
        return false;
    }

    ssize_t maxBody = getMaxBodySize(res);
    String  method  = client->getMethod();

    if ((method == METHOD_GET || method == METHOD_HEAD || method == METHOD_DELETE || method == METHOD_TRACE) && (hasContentLength || isChunked)) {
        sendErrorResponse(client, HTTP_BAD_REQUEST, getHttpStatusMessage(HTTP_BAD_REQUEST), true, 0);
        return false;
    }

    if (isMethodWithBody(method) && !hasContentLength && !isChunked) {
        sendErrorResponse(client, HTTP_LENGTH_REQUIRED, getHttpStatusMessage(HTTP_LENGTH_REQUIRED), true, 0);
        return false;
    }

    if (hasContentLength) {
        size_t cl = client->getContentLength();
        if (maxBody >= 0 && (ssize_t)cl > maxBody) {
            sendErrorResponse(client, HTTP_PAYLOAD_TOO_LARGE, getHttpStatusMessage(HTTP_PAYLOAD_TOO_LARGE), true, 0);
            return false;
        }
    }

    if (maxBody >= 0 && client->getStoreReceiveData().size() > (size_t)maxBody) {
        sendErrorResponse(client, HTTP_PAYLOAD_TOO_LARGE, getHttpStatusMessage(HTTP_PAYLOAD_TOO_LARGE), true, 0);
        return false;
    }
    return true;
}

void ServerManager::handleCgiBodyStreaming(Client* client) {
    bool        isChunked = client->isChunkedEncoding();
    RouteResult boundRes  = getValue(clientRoutes, client->getFd(), RouteResult());
    ssize_t     maxBody   = getMaxBodySize(boundRes);

    if (isChunked) {
        if (maxBody >= 0 && client->getStoreReceiveData().size() > (size_t)maxBody) {
            sendErrorResponse(client, HTTP_PAYLOAD_TOO_LARGE, getHttpStatusMessage(HTTP_PAYLOAD_TOO_LARGE), true, 0);
            return;
        }
        String decoded;
        if (decodeChunkedBody(client->getStoreReceiveData(), decoded)) {
            if (maxBody >= 0 && decoded.size() > (size_t)maxBody) {
                sendErrorResponse(client, HTTP_PAYLOAD_TOO_LARGE, getHttpStatusMessage(HTTP_PAYLOAD_TOO_LARGE), true, 0);
                return;
            }
            client->getCgi().appendBuffer(decoded);
            client->getCgi().setWriteDone(true);
            client->clearStoreReceiveData();
            pollManager.addFd(client->getCgi().getWriteFd(), POLLOUT);
        }
    } else {
        size_t cl            = client->getContentLength();
        size_t totalReceived = client->getCgi().getTotalReceived();
        if (cl < totalReceived) {
            sendErrorResponse(client, HTTP_BAD_REQUEST, getHttpStatusMessage(HTTP_BAD_REQUEST), true, 0);
            return;
        }
        size_t toWrite = minValue(client->getStoreReceiveData().size(), cl - totalReceived);
        if (toWrite > 0) {
            String part = client->getStoreReceiveData().substr(0, toWrite);
            if (maxBody >= 0 && (ssize_t)(totalReceived + part.size()) > maxBody) {
                sendErrorResponse(client, HTTP_PAYLOAD_TOO_LARGE, getHttpStatusMessage(HTTP_PAYLOAD_TOO_LARGE), true, 0);
                return;
            }
            client->getCgi().appendBuffer(part);
            client->removeReceivedData(toWrite);
            if (client->getCgi().getWriteFd() != -1)
                pollManager.addFd(client->getCgi().getWriteFd(), POLLOUT);
        }
        if (client->getCgi().getTotalReceived() >= cl)
            client->getCgi().setWriteDone(true);
    }
}

bool ServerManager::handleRegularBody(Client* client) {
    bool    isChunked = client->isChunkedEncoding();
    ssize_t cl        = client->getContentLength();
    if (!isChunked && client->getStoreReceiveData().size() >= (size_t)cl) {
        if (cl > 0)
            client->getRequest().parseBody(client->getStoreReceiveData().substr(0, cl));
        RouteResult res = getValue(clientRoutes, client->getFd(), RouteResult());
        // res.setRequest(client->getRequest());

        if (res.getHandlerType() == CGI) {
            if (cl <= 0 && !isChunked)
                client->getCgi().setWriteDone(true);
            responseBuilder.build(res, &client->getCgi(), getServerFds());
            if (client->getCgi().isActive()) {
                registerCgiPipes(client);
                return true;
            }
        } else {
            HttpResponse response = responseBuilder.build(res, &client->getCgi(), getServerFds());
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

                return false;
            }

            client->getRequest().parseBody(decoded);
            // res.setRequest(client->getRequest());

            if (res.getHandlerType() == CGI) {
                client->getCgi().appendBuffer(decoded);
                client->getCgi().setWriteDone(true);
                responseBuilder.build(res, &client->getCgi(), getServerFds());
                if (client->getCgi().isActive()) {
                    registerCgiPipes(client);
                    return true;
                }
            } else {
                // client->clearStoreReceiveData();
                HttpResponse response = responseBuilder.build(res, &client->getCgi(), getServerFds());
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
    }
    pollManager.removeFdByValue(clientFd);
    if (c) {
        c->closeConnection();
        delete c;
    }
    clients.erase(clientFd);
    clientToServer.erase(clientFd);
    clientRoutes.erase(clientFd);
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
    if (!client) {
        removeCgiPipe(pipeFd);
        return;
    }
    if (client->getCgi().writeBody(pipeFd)) {
        removeCgiPipe(pipeFd);
        if (client->getCgi().getWriteFd() != -1) {
            close(client->getCgi().getWriteFd());
            client->getCgi().setWriteFd(-1);
        }
    }
}

void ServerManager::handleCgiRead(int pipeFd) {
    Client* client = getValue(clients, getValue(cgiPipeToClient, pipeFd, -1), (Client*)NULL);
    if (!client) {
        removeCgiPipe(pipeFd);
        return;
    }
    if (!client->getCgi().handleRead()) {
        removeCgiPipe(pipeFd);
        if (client->getCgi().getWriteFd() != -1) {
            removeCgiPipe(client->getCgi().getWriteFd());
            close(client->getCgi().getWriteFd());
            client->getCgi().setWriteFd(-1);
        }
        if (client->getCgi().getReadFd() != -1) {
            close(client->getCgi().getReadFd());
            client->getCgi().setReadFd(-1);
        }
        client->getCgi().finish();
        HttpResponse cgiResponse = responseBuilder.buildCgiResponse(client->getCgi());
        if (!client->isKeepAlive())
            cgiResponse.addHeader("Connection", "close");
        else
            cgiResponse.addHeader("Connection", "keep-alive");
        client->setSendData(cgiResponse.toString());
        client->resetForNextRequest();
        clientRoutes.erase(client->getFd());
        pollManager.addFd(client->getFd(), POLLIN | POLLOUT);
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
    MapIntServerPtr::const_iterator it = serverFdMap.find(serverFd);
    if (it != serverFdMap.end())
        return it->second;
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

VectorInt ServerManager::getServerFds() const {
    VectorInt fds;
    for (size_t i = 0; i < servers.size(); i++)
        fds.push_back(servers[i]->getFd());
    return fds;
}
