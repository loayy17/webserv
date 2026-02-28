#include <csignal>
#include <iostream>
#include "config/ConfigParser.hpp"
#include "server/ServerManager.hpp"
#include "utils/Logger.hpp"

volatile sig_atomic_t g_running = 0;

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
    setupSignals();

    String configFile = (ac > 1) ? av[1] : "webserv.conf";
    if (!fileExists(configFile)) {
        Logger::error("Error: Configuration file '" + configFile + "' not found.");
        return 1;
    }
    Logger::info("========================================");
    Logger::info("       Webserv HTTP Server v1.0        ");
    Logger::info("========================================");
    ConfigParser parser(configFile);
    if (!parser.parse())
        return 1;

    VectorServerConfig configs = parser.getServers();
    if (configs.empty()) {
        Logger::error("No server configurations found");
        return 1;
    }

    ServerManager serverManager(configs);

    if (!serverManager.initialize()) {
        Logger::error("Failed to initialize server manager");
        return 1;
    }

    Logger::info("\n========================================");
    Logger::info("  Servers: " + typeToString(serverManager.getServerCount()));
    Logger::info("Server Manager is running...");
    Logger::info("========================================");

    serverManager.run();

    Logger::info("\n========================================");
    Logger::info("       Server Stopped Successfully      ");
    Logger::info("========================================");

    return 0;
}
