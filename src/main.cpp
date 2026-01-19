#include <csignal>
#include <cstring>
#include <iostream>
#include "config/ConfigParser.hpp"
#include "server/ServerManager.hpp"
#include "utils/Logger.hpp"

ServerManager* g_serverManager = NULL;

void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        std::cout << "\nShutdown signal received..." << std::endl;
        if (g_serverManager) {
            g_serverManager->shutdown();
        }
    }
}

void setupSignals() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);
}

int main(int ac, char** av) {
    std::string configFile = (ac > 1) ? av[1] : "webserv.conf";

    std::cout << "========================================" << std::endl;
    std::cout << "       Webserv HTTP Server v1.0        " << std::endl;
    std::cout << "========================================\n" << std::endl;

    ConfigParser parser(configFile);
    if (!parser.parse()) {
        std::cout << "[ERROR]:Config parsing failed: " + parser.getError() << std::endl;
        return 1;
    }

    std::vector<ServerConfig> configs = parser.getServers();
    if (configs.empty()) {
        std::cout << "[ERROR]: No server configurations found" << std::endl;
        return 1;
    }

    ServerManager serverManager;
    g_serverManager = &serverManager;

    if (!serverManager.initialize(configs)) {
        std::cout << "[ERROR]: Failed to initialize server manager" << std::endl;
        return 1;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Servers: " << serverManager.getServerCount() << std::endl;
    std::cout << "  Status: Running" << std::endl;
    std::cout << "========================================\n" << std::endl;

    setupSignals();
    serverManager.run();

    std::cout << "\n========================================" << std::endl;
    std::cout << "       Server Stopped Successfully      " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
