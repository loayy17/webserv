#include <csignal>
#include <iostream>
#include "config/ConfigParser.hpp"
#include "server/ServerManager.hpp"
#include "utils/Logger.hpp"

volatile sig_atomic_t g_running = 1;

void signalHandler(int signum) {
    (void)signum;
    g_running = 0;
}

void setupSignals() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN);
}

int main(int ac, char** av) {
    String configFile = (ac > 1) ? av[1] : "webserv.conf";

    std::cout << "========================================" << std::endl;
    std::cout << "       Webserv HTTP Server v1.0        " << std::endl;
    std::cout << "========================================\n" << std::endl;

    ConfigParser parser(configFile);
    if (!parser.parse()) {
        return 1;
    }

    VectorServerConfig configs = parser.getServers();
    if (configs.empty()) {
        std::cout << "[ERROR]: No server configurations found" << std::endl;
        return 1;
    }

    ServerManager serverManager(configs);

    if (!serverManager.initialize()) {
        std::cout << "[ERROR]: Failed to initialize server manager" << std::endl;
        return 1;
    }

    std::cout << "\n========================================" << std::endl;
    Logger::info("  Servers: " + typeToString(serverManager.getServerCount()));
    Logger::info("Server Manager is running...");
    std::cout << "========================================\n" << std::endl;

    setupSignals();
    serverManager.run();

    std::cout << "\n========================================" << std::endl;
    std::cout << "       Server Stopped Successfully      " << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
