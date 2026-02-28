#include "ServerManager.hpp"
#include <fcntl.h>
#include <netinet/tcp.h>

ServerManager::ServerManager()
    : pollManager(), servers(), serverConfigs(), clients(), clientToServer(), serverToConfigs(), mimeTypes(), responseBuilder(mimeTypes), sessionManager() {
    pollManager.reserve(MAX_CONNECTIONS + 10);
}

ServerManager::ServerManager(const VectorServerConfig& _configs)
    : pollManager(), servers(), serverConfigs(_configs), clients(), clientToServer(), serverToConfigs(), mimeTypes(), responseBuilder(mimeTypes), sessionManager() {
    pollManager.reserve(MAX_CONNECTIONS + 10);
}

ServerManager::~ServerManager() {
    shutdown();
}

bool ServerManager::initialize() {
    if (serverConfigs.empty())
        return Logger::error("No server configurations provided");
    if (!initializeServers(serverConfigs) || servers.empty())
        return Logger::error("Failed to initialize servers");
    g_running = 1;
    return Logger::info("ServerManager initialized with Optimized Poll");
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
        int fd = server->getFd();
        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        fcntl(fd, F_SETFL, O_NONBLOCK);
    } catch (...) {
        Logger::error("Server init failed for listen " + serverConfig.getListenAddresses()[listenIndex].getListenAddress());
        if (server) delete server;
        return NULL;
    }
    return server;
}

Server* ServerManager::createServerForListener(const String& listenerKey, const VectorServerConfig& serversConfigsForListener) {
    if (serversConfigsForListener.empty())
        return NULL;
    const ServerConfig& firstConfig = serversConfigsForListener[0];
    const VectorListenAddress& addresses = firstConfig.getListenAddresses();
    size_t listenIndex = 0;
    for (size_t i = 0; i < addresses.size(); i++) {
        if (addresses[i].getListenAddress() == listenerKey) {
            listenIndex = i;
            break;
        }
    }
    Server* server = initializeServer(firstConfig, listenIndex);
    if (!server) return NULL;
    pollManager.addFd(server->getFd(), POLLIN);
    return server;
}

bool ServerManager::initializeServers(const VectorServerConfig& serversConfigs) {
    ListenerToConfigsMap listenerToConfigs = mapListenersToConfigs(serversConfigs);
    for (ListenerToConfigsMap::iterator it = listenerToConfigs.begin(); it != listenerToConfigs.end(); ++it) {
        Server* server = createServerForListener(it->first, it->second);
        if (!server) continue;
        servers.push_back(server);
        serverToConfigs[server->getFd()] = it->second;
    }
    return !servers.empty();
}

String ServerManager::getCachedDate() {
    static time_t lastUpdate = 0;
    static String cachedDate;
    time_t now = getCurrentTime();
    if (now != lastUpdate) {
        lastUpdate = now;
        cachedDate = getHttpDate(now);
    }
    return cachedDate;
}

bool ServerManager::run() {
    if (!g_running) return false;
    while (g_running) {
        int eventCount = pollManager.pollConnections(100);
        checkTimeouts(CLIENT_TIMEOUT);
        reapCgiProcesses();

        if (eventCount <= 0) continue;
        for (size_t i = 0; i < pollManager.size() && eventCount > 0; i++) {
            int fd = pollManager.getFd(i);
            bool hasIn = pollManager.hasEvent(i, POLLIN);
            bool hasOut = pollManager.hasEvent(i, POLLOUT);
            bool hasErr = pollManager.hasEvent(i, POLLERR | POLLHUP);
            if (!hasIn && !hasOut && !hasErr) continue;
            eventCount--;

            if (isCgiPipe(fd)) {
                if (hasOut) handleCgiWrite(fd);
                if (hasIn || hasErr) handleCgiRead(fd);
                if (i < pollManager.size() && pollManager.getFd(i) != fd) --i;
                continue;
            }
            if (hasIn) {
                if (isServerSocket(fd)) {
                    Server* s = findServerByFd(fd);
                    if (s) acceptNewConnection(s);
                } else if (clients.count(fd)) handleClientRead(fd);
            }
            if (hasOut && clients.count(fd)) handleClientWrite(fd);
            if (hasErr && clients.count(fd)) closeClientConnection(fd);
            if (i < pollManager.size() && pollManager.getFd(i) != fd) --i;
        }
    }
    return true;
}

bool ServerManager::acceptNewConnection(Server* server) {
    String addr;
    int fd = server->acceptConnection(addr);
    if (fd < 0) return false;
    if (clients.size() >= MAX_CONNECTIONS) {
        close(fd);
        return false;
    }
    fcntl(fd, F_SETFL, O_NONBLOCK);
    int opt = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
    Client* c = new Client(fd);
    c->setRemoteAddress(addr);
    clients[fd] = c;
    clientToServer[fd] = server;
    pollManager.addFd(fd, POLLIN);
    return true;
}

void ServerManager::handleClientRead(int fd) {
    Client* c = clients[fd];
    if (c->receiveData() <= 0) {
        closeClientConnection(fd);
        return;
    }
    processRequest(c, clientToServer[fd]);
}

void ServerManager::handleClientWrite(int fd) {
    Client* c = clients[fd];
    if (c->sendData() < 0) {
        closeClientConnection(fd);
        return;
    }
    if (c->getStoreSendData().empty()) {
        if (!c->isKeepAlive()) closeClientConnection(fd);
        else pollManager.addFd(fd, POLLIN);
    }
}

void ServerManager::processRequest(Client* client, Server* server) {
    while (true) {
        if (!client->isHeadersParsed()) {
            String& buf = client->getStoreReceiveData();
            size_t pos = buf.find(DOUBLE_CRLF);
            size_t len = 4;
            if (pos == String::npos) { pos = buf.find("\n\n"); len = 2; }
            if (pos == String::npos) {
                if (buf.size() > MAX_HEADER_SIZE) sendErrorResponse(client, 431, "Headers Too Large", true, 0);
                return;
            }
            if (pos > MAX_HEADER_SIZE) { sendErrorResponse(client, 431, "Headers Too Large", true, 0); return; }

            String head = buf.substr(0, pos);
            client->removeReceivedData(pos + len);
            if (!client->getRequest().parseHeaders(head)) { sendErrorResponse(client, 400, "Bad Request", true, 0); return; }
            client->getRequest().setPort(server->getPort());
            client->setHeadersParsed(true);

            String conn = toLowerWords(client->getRequest().getHeader("Connection"));
            client->setKeepAlive(client->getRequest().getHttpVersion() == "HTTP/1.1" && conn.find("close") == String::npos);
        }

        if (client->isHeadersParsed()) {
            Router router(serverToConfigs[server->getFd()], client->getRequest());
            RouteResult res = router.processRequest();
            res.setRemoteAddress(client->getRemoteAddress());

            if (res.getHandlerType() == CGI) {
                if (client->getCgi().isActive()) {
                    String& buf = client->getStoreReceiveData();
                    if (!buf.empty()) {
                        client->getCgi().appendBuffer(buf);
                        buf.clear();
                        pollManager.addFd(client->getCgi().getWriteFd(), POLLOUT);
                    }
                    ssize_t cl = client->getRequest().getContentLength();
                    if (cl >= 0 && client->getRequest().getBody().size() >= (size_t)cl) client->getCgi().setWriteDone();
                } else {
                    ssize_t cl = client->getRequest().getContentLength();
                    bool isChunked = toLowerWords(client->getRequest().getHeader("Transfer-Encoding")).find("chunked") != String::npos;
                    responseBuilder.build(res, &client->getCgi(), VectorInt());
                    if (client->getCgi().isActive()) {
                        registerCgiPipes(client);
                        if (cl <= 0 && !isChunked) client->getCgi().setWriteDone();
                        continue;
                    }
                }
            } else {
                ssize_t cl = client->getRequest().getContentLength();
                if (cl > 0 && client->getStoreReceiveData().size() < (size_t)cl) return;
                if (cl > 0) {
                    client->getRequest().parseBody(client->getStoreReceiveData().substr(0, cl));
                    client->removeReceivedData(cl);
                }
                HttpResponse response = responseBuilder.build(res, &client->getCgi(), VectorInt());
                response.addHeader("Date", getCachedDate());
                if (!client->isKeepAlive()) response.addHeader("Connection", "close");
                client->setSendData(response.toString());
                client->setHeadersParsed(false);
                client->getRequest().clear();
                pollManager.addFd(client->getFd(), POLLIN | POLLOUT);
            }
        }
        break;
    }
}

void ServerManager::registerCgiPipes(Client* client) {
    CgiProcess& cgi = client->getCgi();
    if (!cgi.isWriteDone()) {
        pollManager.addFd(cgi.getWriteFd(), POLLOUT);
        cgiPipeToClient[cgi.getWriteFd()] = client->getFd();
    }
    pollManager.addFd(cgi.getReadFd(), POLLIN);
    cgiPipeToClient[cgi.getReadFd()] = client->getFd();
}

void ServerManager::handleCgiWrite(int pipeFd) {
    int clientFd = cgiPipeToClient[pipeFd];
    Client* c = clients[clientFd];
    if (c->getCgi().writeBody(pipeFd)) {
        pollManager.removeFdByValue(pipeFd);
        cgiPipeToClient.erase(pipeFd);
    }
}

void ServerManager::handleCgiRead(int pipeFd) {
    int clientFd = cgiPipeToClient[pipeFd];
    Client* c = clients[clientFd];
    if (!c->getCgi().handleRead()) {
        pollManager.removeFdByValue(pipeFd);
        cgiPipeToClient.erase(pipeFd);
        HttpResponse resp = responseBuilder.buildCgiResponse(c->getCgi());
        resp.addHeader("Date", getCachedDate());
        c->setSendData(resp.toString());
        c->setHeadersParsed(false);
        c->getRequest().clear();
        pollManager.addFd(clientFd, POLLIN | POLLOUT);
    }
}

void ServerManager::closeClientConnection(int fd) {
    Client* c = clients[fd];
    if (c->getCgi().isActive()) cleanupClientCgi(c);
    pollManager.removeFdByValue(fd);
    delete c;
    clients.erase(fd);
    clientToServer.erase(fd);
}

void ServerManager::cleanupClientCgi(Client* client) {
    if (client->getCgi().getWriteFd() != -1) removeCgiPipe(client->getCgi().getWriteFd());
    if (client->getCgi().getReadFd() != -1) removeCgiPipe(client->getCgi().getReadFd());
    client->getCgi().cleanup();
}

void ServerManager::removeCgiPipe(int fd) {
    pollManager.removeFdByValue(fd);
    cgiPipeToClient.erase(fd);
}

void ServerManager::reapCgiProcesses() {
    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
        Client* c = it->second;
        if (!c->getCgi().isActive()) continue;
        int status;
        if (waitpid(c->getCgi().getPid(), &status, WNOHANG) > 0) {
            c->getCgi().setExited(status);
        }
    }
}

void ServerManager::checkTimeouts(int timeout) {
    std::vector<int> toClose;
    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->isTimedOut(timeout)) toClose.push_back(it->first);
    }
    for (size_t i = 0; i < toClose.size(); i++) closeClientConnection(toClose[i]);
}

void ServerManager::sendErrorResponse(Client* client, int code, const String& msg, bool closeConn, size_t) {
    HttpResponse res = responseBuilder.buildError(code, msg);
    res.addHeader("Date", getCachedDate());
    if (closeConn) {
        res.addHeader("Connection", "close");
        client->setKeepAlive(false);
    }
    client->setSendData(res.toString());
    pollManager.addFd(client->getFd(), POLLIN | POLLOUT);
}

bool ServerManager::isServerSocket(int fd) const {
    for (size_t i = 0; i < servers.size(); i++) if (servers[i]->getFd() == fd) return true;
    return false;
}

bool ServerManager::isCgiPipe(int fd) const {
    return cgiPipeToClient.count(fd);
}

Server* ServerManager::findServerByFd(int fd) const {
    for (size_t i = 0; i < servers.size(); i++) if (servers[i]->getFd() == fd) return servers[i];
    return NULL;
}

void ServerManager::shutdown() {
    g_running = 0;
    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
        delete it->second;
    }
    clients.clear();
    for (size_t i = 0; i < servers.size(); i++) delete servers[i];
    servers.clear();
}

size_t ServerManager::getServerCount() const { return servers.size(); }
size_t ServerManager::getClientCount() const { return clients.size(); }
