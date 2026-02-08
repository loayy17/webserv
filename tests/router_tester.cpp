#include <fstream>
#include <iostream>
#include <sstream>
#include "../src/config/ConfigParser.hpp"
#include "../src/http/HttpRequest.hpp"
#include "../src/http/Router.hpp"

// Read file content into string
String readFile(const String& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "ERROR|Cannot open file: " << filename << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <config_file> <request_file>" << std::endl;
        return 1;
    }

    String configFile  = argv[1];
    String requestFile = argv[2];

    // 1. Parse config
    ConfigParser parser(configFile);
    if (!parser.parse()) {
        return 1;
    }

    std::vector<ServerConfig> servers = parser.getServers();
    if (servers.empty()) {
        std::cout << "ERROR|No servers in config" << std::endl;
        return 1;
    }

    // 2. Parse request
    String rawRequest = readFile(requestFile);
    if (rawRequest.empty()) {
        return 1;
    }

    HttpRequest request;
    if (!request.parse(rawRequest)) {
        std::cout << "ERROR|Request parsing failed" << std::endl;
        return 1;
    }

    // 3. Create router and process
    Router router(servers, request);
    router.processRequest();

    // 4. Output results
    std::cout << "isPathFound=" << (router.getIsPathFound() ? "true" : "false") << std::endl;
    std::cout << "statusCode=" << router.getStatusCode() << std::endl;
    std::cout << "matchedPath=" << router.getMatchedPath() << std::endl;
    std::cout << "serverName=" << (router.getServer() ? router.getServer()->getServerName() : "") << std::endl;
    std::cout << "isRedirect=" << (router.getIsRedirect() ? "true" : "false") << std::endl;
    std::cout << "redirectUrl=" << router.getRedirectUrl() << std::endl;
    std::cout << "pathRootUri=" << router.getPathRootUri() << std::endl;
    std::cout << "remainingPath=" << router.getRemainingPath() << std::endl;

    return 0;
}
