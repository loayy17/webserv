#include <fstream>
#include <iostream>
#include <sstream>
#include <csignal>
#include "../src/config/ConfigParser.hpp"
#include "../src/http/HttpRequest.hpp"
#include "../src/http/Router.hpp"
volatile sig_atomic_t g_running = 1;
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
    RouteResult result = router.processRequest();

    // 4. Output results
    std::cout << "statusCode=" << result.getStatusCode() << std::endl;
    std::cout << "matchedPath=" << result.getMatchedPath() << std::endl;
    std::cout << "serverName=" << (result.getServer() ? result.getServer()->getServerName() : "") << std::endl;
    std::cout << "isRedirect=" << (result.getIsRedirect() ? "true" : "false") << std::endl;
    std::cout << "redirectUrl=" << result.getRedirectUrl() << std::endl;
    std::cout << "pathRootUri=" << result.getPathRootUri() << std::endl;
    std::cout << "remainingPath=" << result.getRemainingPath() << std::endl;
    std::cout << "isCgiRequest=" << (result.getIsCgiRequest() ? "true" : "false") << std::endl;
    std::cout << "isUploadRequest=" << (result.getIsUploadRequest() ? "true" : "false") << std::endl;
    std::cout << "errorMessage=" << result.getErrorMessage() << std::endl;

    return 0;
}
