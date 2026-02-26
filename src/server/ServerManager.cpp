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
    return Logger::info("ServerManager initialized");
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
        Logger::error("Server init failed for listen " + serverConfig.getListenAddresses()[listenIndex].getListenAddress() + ": " +
                      "Address already in use or insufficient permissions");
        if (server) {
            if (server->getFd() >= 0 && close(server->getFd()) == -1)
                Logger::error("Failed to close server fd: the fd might not have been created or already closed");
            delete server;
        }
        return NULL;
    }
    return server;
}

Server* ServerManager::createServerForListener(const String& listenerKey, const VectorServerConfig& serversConfigsForListener,
                                               PollManager& pollManager) {
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
    pollManager.addFd(server->getFd(), POLLIN);
    return server;
}

bool ServerManager::initializeServers(const VectorServerConfig& serversConfigs) {
    ListenerToConfigsMap      listenerToConfigs = mapListenersToConfigs(serversConfigs);
    std::map<String, Server*> listenerToServerMap;
    for (ListenerToConfigsMap::iterator it = listenerToConfigs.begin(); it != listenerToConfigs.end(); ++it) {
        const String&             listenerKey               = it->first;
        const VectorServerConfig& serversConfigsForListener = it->second;
        Server*                   server                    = createServerForListener(listenerKey, serversConfigsForListener, pollManager);
        if (!server)
            continue;
        servers.push_back(server);
        listenerToServerMap[listenerKey] = server;
        serverToConfigs[server->getFd()] = serversConfigsForListener;
    }
    return !servers.empty();
}

bool ServerManager::run() {
    if (!g_running)
        return Logger::error("Cannot run server manager");
    time_t lastSessionCleanup = getCurrentTime();
    while (g_running) {
        int eventCount = pollManager.pollConnections(100);
        checkTimeouts(CLIENT_TIMEOUT);
        // Reap any finished CGI children without blocking
        reapCgiProcesses();
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
            // server or client socket events
            if (hasIn) {
                if (isServerSocket(fd)) {
                    Server* server = findServerByFd(fd);
                    if (server)
                        acceptNewConnection(server);
                } else if (clients.find(fd) != clients.end())
                    handleClientRead(fd);
            }
            // client socket ready to send data
            if (hasOut && clients.find(fd) != clients.end())
                handleClientWrite(fd);

            if (i < pollManager.size() && pollManager.getFd(i) != fd)
                --i;
            eventCount--;
        }
    }
    return true;
}

void ServerManager::reapCgiProcesses() {
    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
        Client* client = it->second;
        if (!client->getCgi().isActive())
            continue;
        CgiProcess& cgi = client->getCgi();
        int status = 0;
        pid_t ret = waitpid(cgi.getPid(), &status, WNOHANG);
        if (ret > 0) {
            cgi.setExited(status);
            // if both pipes done, build and send response
            if (cgi.isReadDone() && cgi.isWriteDone()) {
                ResponseBuilder builder(mimeTypes);
                HttpResponse response = builder.buildCgiResponse(cgi);
                client->setSendData(response.toString());
                pollManager.addFd(client->getFd(), POLLIN | POLLOUT);
            }
        }
    }
}

bool ServerManager::acceptNewConnection(Server* server) {
    String remoteAddress;
    int    clientFd = server->acceptConnection(remoteAddress);
    if (clientFd < 0)
        return Logger::error("Failed to accept new connection");

    if (clients.size() >= MAX_CONNECTIONS) {
        Logger::error("Connection limit reached (" + typeToString<size_t>(MAX_CONNECTIONS) + "), rejecting new connection");
        close(clientFd);
        return false;
    }

    Client* client = NULL;
    try {
        client = new Client(clientFd);
        client->setRemoteAddress(remoteAddress);
    } catch (const std::bad_alloc& e) {
        Logger::error("Memory allocation failed for client");
        close(clientFd);
        return false;
    }
    clients[clientFd]        = client;
    clientToServer[clientFd] = server;
    pollManager.addFd(clientFd, POLLIN);
    return Logger::info("Connection accepted on port " + typeToString(server->getPort()));
}

void ServerManager::handleClientRead(int clientFd) {
    Client* client = getValue(clients, clientFd, (Client*)NULL);
    if (client == NULL) {
        closeClientConnection(clientFd);
        return;
    }

    ssize_t received = client->receiveData();
    if (received == 0) { // 0 = client disconnected gracefully
        closeClientConnection(clientFd);
        return;
    }
    if (received < 0)
        return;
    if (client->getCgi().isActive())
        return;
    Server* server = getValue(clientToServer, clientFd, (Server*)NULL);
    if (server) {
        processRequest(client, server);
    }
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

    if (client->getStoreSendData().empty()) {
        if (client->isKeepAlive()) {
            pollManager.addFd(clientFd, POLLIN);
            return;
        }
        closeClientConnection(clientFd);
    }
}

void ServerManager::checkTimeouts(int timeout) {
    std::vector<int> toClose;

    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getCgi().isActive()) {
            if (getDifferentTime(it->second->getCgi().getStartTime(), getCurrentTime()) > CGI_TIMEOUT) {
                Logger::info("CGI timeout, killing process");
                cleanupClientCgi(it->second);
                ResponseBuilder builder(mimeTypes);
                HttpResponse    response = builder.buildError(HTTP_GATEWAY_TIMEOUT, "CGI Timeout");
                it->second->setSendData(response.toString());
                pollManager.addFd(it->first, POLLIN | POLLOUT);
            }
            // timeout for client connections
        } else if (it->second->isTimedOut(timeout)) {
            toClose.push_back(it->first);
        }
    }

    for (size_t i = 0; i < toClose.size(); i++) {
        Logger::info("Client timeout, closing connection");
        closeClientConnection(toClose[i]);
    }
}

void ServerManager::sendErrorResponse(Client* client, int statusCode, const String& message, bool closeConnection, size_t bytesToRemove) {
    ResponseBuilder builder(mimeTypes);
    HttpResponse    response = builder.buildError(statusCode, message);
    if (closeConnection) {
        response.addHeader(HEADER_CONNECTION, "close");
        client->setKeepAlive(false);
    }
    client->setSendData(response.toString());

    if (bytesToRemove > 0)
        client->removeReceivedData(bytesToRemove);
    else
        client->clearStoreReceiveData();

    pollManager.addFd(client->getFd(), POLLIN | POLLOUT);
}

void ServerManager::processRequest(Client* client, Server* server) {
    const String& buffer   = client->getStoreReceiveData();
    size_t        dataSize = buffer.size();

    // 1. Detect end of headers
    size_t headerEnd    = buffer.find(DOUBLE_CRLF);
    size_t headerEndLen = 4;

    if (headerEnd == String::npos) {
        headerEnd    = buffer.find("\n\n");
        headerEndLen = 2;
    }

    if (headerEnd == String::npos) {
        if (dataSize > MAX_HEADER_SIZE) {
            sendErrorResponse(client, HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE, getHttpStatusMessage(HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE), true, 0);
        }
        return;
    }

    if (headerEnd > MAX_HEADER_SIZE) {
        sendErrorResponse(client, HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE, getHttpStatusMessage(HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE), true,
                          headerEnd + headerEndLen);
        return;
    }

    String headerSection = buffer.substr(0, headerEnd);

    bool    isChunked        = isChunkedTransferEncoding(headerSection);
    ssize_t contentLength    = 0;
    bool    hasContentLength = extractContentLength(contentLength, headerSection);

    if (hasContentLength && isChunked) {
        sendErrorResponse(client, HTTP_BAD_REQUEST, getHttpStatusMessage(HTTP_BAD_REQUEST), true, 0);
        return;
    }

    size_t requestSize = 0;
    String fullRequest;

    // 2. Handle body (Chunked)

    if (isChunked) {
        String bodyPart   = buffer.substr(headerEnd + headerEndLen);
        size_t finalChunk = bodyPart.find("\r\n0\r\n");
        if (finalChunk == String::npos) {
            if (bodyPart.find("0\r\n\r\n") == 0)
                finalChunk = 0;
            else
                return;
        }

        size_t endMarker = bodyPart.find("\r\n\r\n", finalChunk);
        if (endMarker == String::npos)
            return;

        requestSize = headerEnd + headerEndLen + endMarker + 4;
        if (dataSize < requestSize)
            return;

        String chunkedBody = bodyPart.substr(0, endMarker + 4);
        String decodedBody;

        if (!decodeChunkedBody(chunkedBody, decodedBody)) {
            sendErrorResponse(client, HTTP_BAD_REQUEST, getHttpStatusMessage(HTTP_BAD_REQUEST), true, 0);
            return;
        }

        fullRequest = headerSection + "\r\nContent-Length: " + typeToString<size_t>(decodedBody.size()) + "\r\n\r\n" + decodedBody;
    }
    // 3. Handle Content-Length
    else {
        if (hasContentLength) {
            if (contentLength < 0) {
                sendErrorResponse(client, HTTP_BAD_REQUEST, getHttpStatusMessage(HTTP_BAD_REQUEST), true, 0);
                return;
            }

            if (static_cast<size_t>(contentLength) > SIZE_MAX - headerEnd - headerEndLen) {
                sendErrorResponse(client, HTTP_BAD_REQUEST, getHttpStatusMessage(HTTP_BAD_REQUEST), true, 0);
                return;
            }
        }

        requestSize = headerEnd + headerEndLen + (hasContentLength ? static_cast<size_t>(contentLength) : 0);

        if (dataSize < requestSize)
            return;

        fullRequest = buffer.substr(0, requestSize);
    }

    // 4. Parse HTTP request
    HttpRequest request;
    if (!request.parse(fullRequest)) {
        sendErrorResponse(client, request.getErrorCode(), getHttpStatusMessage(request.getErrorCode()), true, requestSize);
        return;
    }

    request.setPort(server->getPort());

    // 5. Session Handling (BEFORE routing)
    String         sessionId = request.getCookie(SESSION_COOKIE_NAME);
    SessionResult* session   = NULL;

    if (!sessionId.empty())
        session = sessionManager.getSession(sessionId);

    String newSessionId;

    if (!session && request.getMethod() == "POST" && !request.getBody().empty()) {
        String       body = request.getBody();
        String       username;
        VectorString pairs;

        splitByString(body, pairs, "&");

        for (size_t i = 0; i < pairs.size(); ++i) {
            String key, val;
            if (splitByChar(pairs[i], key, val, '=') && trimSpaces(key) == "username") {
                username = trimSpaces(val);
                break;
            }
        }

        if (!username.empty()) {
            try {
                newSessionId = sessionManager.createSession(username);
                session      = sessionManager.getSession(newSessionId);
            } catch (const std::exception& e) {
                Logger::error("Failed to create session: " + String(e.what()));
            }
        }
    }

    // 6. Routing
    Router      router(serverToConfigs[server->getFd()], request);
    RouteResult routerResult = router.processRequest();
    routerResult.setRemoteAddress(client->getRemoteAddress());

    // Body size check against location config
    const LocationConfig* loc    = routerResult.getLocation();
    ssize_t               locMax = (loc ? loc->getClientMaxBody() : -1);

    if (locMax != -1 && static_cast<ssize_t>(request.getBody().size()) > locMax) {
        sendErrorResponse(client, HTTP_PAYLOAD_TOO_LARGE, getHttpStatusMessage(HTTP_PAYLOAD_TOO_LARGE), true, requestSize);
        return;
    }

    // 7. Keep-Alive logic
    bool keepAlive = (request.getHttpVersion() == HTTP_VERSION_1_1);

    String connHeader = request.getHeader(HEADER_CONNECTION);
    if (!connHeader.empty()) {
        String       connLower = toLowerWords(connHeader);
        VectorString connValues;
        splitByString(connLower, connValues, ",");
        for (size_t i = 0; i < connValues.size(); ++i) {
            std::string token = trimSpaces(toLowerWords(connValues[i]));
            if (token == "close") {
                keepAlive = false;
                break;
            } else if (token == "keep-alive")
                keepAlive = true;
        }
    }

    client->setKeepAlive(keepAlive);
    client->incrementRequestCount();

    // Enforce keepalive request limit
    if (keepAlive && client->getRequestCount() >= MAX_KEEPALIVE_REQUESTS) {
        keepAlive = false;
        client->setKeepAlive(false);
    }

    // 8. Build Response
    ResponseBuilder builder(mimeTypes);
    HttpResponse    response = builder.build(routerResult, &client->getCgi(), pollManager.getFds());

    // Match response HTTP version to request version
    response.setHttpVersion(request.getHttpVersion());

    if (keepAlive)
        response.addHeader(HEADER_CONNECTION, "keep-alive");
    else
        response.addHeader(HEADER_CONNECTION, "close");

    if (!newSessionId.empty())
        response.addSetCookie(SessionManager::buildSetCookieHeader(newSessionId));

    // 9. CGI Handling
    if (client->getCgi().isActive()) {
        registerCgiPipes(client);
        client->removeReceivedData(requestSize);
        return;
    }

    // 10. Send Response
    client->setSendData(response.toString());
    client->removeReceivedData(requestSize);

    pollManager.addFd(client->getFd(), POLLIN | POLLOUT);
}

void ServerManager::closeClientConnection(int clientFd) {
    Client* c = getValue(clients, clientFd, (Client*)NULL);
    if (c && c->getCgi().isActive())
        cleanupClientCgi(c);

    for (size_t i = 0; i < pollManager.size(); i++) {
        if (pollManager.getFd(i) == clientFd) {
            pollManager.removeFd(i);
            break;
        }
    }
    if (c) {
        c->closeConnection();
        delete c;
    }
    clients.erase(clientFd);
    clientToServer.erase(clientFd);
}

bool ServerManager::isCgiPipe(int fd) const {
    return cgiPipeToClient.find(fd) != cgiPipeToClient.end();
}

void ServerManager::removeCgiPipe(int pipeFd) {
    for (size_t i = 0; i < pollManager.size(); i++) {
        if (pollManager.getFd(i) == pipeFd) {
            pollManager.removeFd(i);
            break;
        }
    }
    cgiPipeToClient.erase(pipeFd);
}

void ServerManager::registerCgiPipes(Client* client) {
    CgiProcess& cgi      = client->getCgi();
    int         clientFd = client->getFd();
    if (!cgi.isWriteDone()) {
        pollManager.addFd(cgi.getWriteFd(), POLLOUT);
        cgiPipeToClient[cgi.getWriteFd()] = clientFd;
    } else {
        close(cgi.getWriteFd());
        cgi.setWriteFd(-1);
    }
    pollManager.addFd(cgi.getReadFd(), POLLIN);
    cgiPipeToClient[cgi.getReadFd()] = clientFd;
    Logger::info("CGI process started (pid " + typeToString<int>(cgi.getPid()) + ")");
}

void ServerManager::handleCgiWrite(int pipeFd) {
    std::map<int, int>::iterator it = cgiPipeToClient.find(pipeFd);
    if (it == cgiPipeToClient.end())
        return;
    Client* client = getValue(clients, it->second, (Client*)NULL);
    if (!client || !client->getCgi().isActive()) {
        removeCgiPipe(pipeFd);
        return;
    }
    if (client->getCgi().writeBody(pipeFd)) {
        removeCgiPipe(pipeFd);
        close(pipeFd);
        client->getCgi().setWriteFd(-1);
        client->getCgi().setWriteDone();
        // If child already exited and read is done, build response
        if (client->getCgi().hasExited() && client->getCgi().isReadDone()) {
            ResponseBuilder builder(mimeTypes);
            client->setSendData(builder.buildCgiResponse(client->getCgi()).toString());
            pollManager.addFd(client->getFd(), POLLIN | POLLOUT);
        }
    }
}

void ServerManager::handleCgiRead(int pipeFd) {
    std::map<int, int>::iterator it = cgiPipeToClient.find(pipeFd);
    if (it == cgiPipeToClient.end())
        return;
    Client* client = getValue(clients, it->second, (Client*)NULL);
    if (!client || !client->getCgi().isActive()) {
        removeCgiPipe(pipeFd);
        return;
    }
    if (!client->getCgi().handleRead()) {
        removeCgiPipe(pipeFd);
        close(pipeFd);
        client->getCgi().setReadDone();
       if (client->getCgi().hasExited() && client->getCgi().isWriteDone()) {
            ResponseBuilder builder(mimeTypes);
            client->setSendData(builder.buildCgiResponse(client->getCgi()).toString());
            pollManager.addFd(client->getFd(), POLLIN | POLLOUT);
        }
    }
}

void ServerManager::cleanupClientCgi(Client* client) {
    if (!client->getCgi().isActive())
        return;
    if (client->getCgi().getWriteFd() != -1)
        removeCgiPipe(client->getCgi().getWriteFd());
    if (client->getCgi().getReadFd() != -1)
        removeCgiPipe(client->getCgi().getReadFd());
    client->getCgi().cleanup();
}

// ─── Server helpers ──────────────────────────────────────────────────────────

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
    // Idempotent: if already cleaned up, skip
    if (servers.empty() && clients.empty())
        return;

    Logger::info("Shutting down...");
    g_running = 0;

    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getCgi().isActive())
            cleanupClientCgi(it->second);
        it->second->closeConnection();
        delete it->second;
    }
    clients.clear();
    clientToServer.clear();
    cgiPipeToClient.clear();

    for (size_t i = 0; i < servers.size(); i++) {
        servers[i]->stop();
        delete servers[i];
    }
    servers.clear();
    Logger::info("Shutdown complete");
}

size_t ServerManager::getServerCount() const {
    return servers.size();
}

size_t ServerManager::getClientCount() const {
    return clients.size();
}
