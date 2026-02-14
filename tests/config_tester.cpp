#include <csignal>
#include <iostream>
#include "../src/config/ConfigParser.hpp"

bool g_running = true;

/* ----------------------------------------------------
 * Helper printer
 * ---------------------------------------------------- */
void printLine() {
    std::cout << "----------------------------------------\n";
}

/* ----------------------------------------------------
 * Print location
 * ---------------------------------------------------- */
void printLocation(const LocationConfig& loc, const String& resolvedBody) {
    std::cout << "  Location: " << loc.getPath() << "\n";
    std::cout << "    root       : " << loc.getRoot() << "\n";
    std::cout << "    autoindex  : " << (loc.getAutoIndex() ? "on" : "off") << "\n";
    std::cout << "    return     : " << (loc.getIsRedirect() ? (loc.getRedirectValue() + " " + typeToString<int>(loc.getRedirectCode())) : "none") << "\n";
    for (size_t i = 0; i < loc.getAllowedMethods().size(); i++) {
        std::cout << "    method     : " << loc.getAllowedMethods()[i] << "\n";
    }
    if (loc.getClientMaxBody().empty() == false)
        std::cout << "    client_max : " << loc.getClientMaxBody() << " (location)\n";
    else
        std::cout << "    client_max : " << resolvedBody << " (fallback)\n";
}

/* ----------------------------------------------------
 * Print server
 * ---------------------------------------------------- */
void printServer(const ServerConfig& srv, const ConfigParser& http) {
    printLine();
    std::cout << "Server\n";
    std::cout << "  listen       : " << srv.getPort() << "\n";
    std::cout << "  server_name  : " << srv.getServerName() << "\n";
    std::cout << "  root         : " << srv.getRoot() << "\n";

    if (srv.getClientMaxBody().empty() == false)
        std::cout << "  client_max   : " << srv.getClientMaxBody() << " (server)\n";
    else
        std::cout << "  client_max   : " << http.getHttpClientMaxBody() << " (http fallback)\n";

    const std::vector<LocationConfig>& locs = srv.getLocations();

    for (size_t i = 0; i < locs.size(); i++) {
        String resolved = http.getHttpClientMaxBody();

        if (srv.getClientMaxBody().empty() == false)
            resolved = srv.getClientMaxBody();

        if (locs[i].getClientMaxBody().empty() == false)
            resolved = locs[i].getClientMaxBody();

        printLocation(locs[i], resolved);
    }
}

/* ----------------------------------------------------
 * MAIN TESTER
 * ---------------------------------------------------- */
int main(int ac, char** av) {
    if (ac != 2) {
        std::cerr << "Usage: ./webserv <config_file>\n";
        return 1;
    }

    ConfigParser parser(av[1]);
    if (!parser.parse()) {
        return 1;
    }

    const std::vector<ServerConfig>& servers = parser.getServers();

    /* ------------------------------------------------
     * Global HTTP
     * ------------------------------------------------ */
    printLine();
    std::cout << "HTTP\n";
    std::cout << "  client_max_body_size : " << parser.getHttpClientMaxBody() << "\n";

    /* ------------------------------------------------
     * Servers
     * ------------------------------------------------ */
    if (servers.empty()) {
        std::cerr << "❌ ERROR: No server block found\n";
        return 1;
    }

    for (size_t i = 0; i < servers.size(); i++) {
        printServer(servers[i], parser);
    }

    printLine();
    std::cout << "✅ CONFIG OK\n";

    return 0;
}
