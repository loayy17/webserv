#include <fstream>
#include <iostream>
#include <sstream>
#include "../src/http/HttpRequest.hpp"

// Read entire file into a string
String readFile(const String& filename) {
    std::ifstream file(filename.c_str(), std::ios::binary); // binary mode to preserve \r\n
    if (!file.is_open()) {
        std::cerr << "ERROR|Cannot open file: " << filename << std::endl;
        return "";
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <request_file.http>" << std::endl;
        return 1;
    }
    String requestFile = argv[1];
    String rawRequest  = readFile(requestFile);
    // Parse request
    HttpRequest request;
    bool        parseResult = request.parse(rawRequest);

    // Output in parseable format
    std::cout << "parseResult=" << (parseResult ? "true" : "false") << std::endl;
    if (parseResult) {
        std::cout << "method=" << request.getMethod() << std::endl;
        std::cout << "uri=" << request.getUri() << std::endl;
        std::cout << "host=" << request.getHost() << std::endl;
        std::cout << "port=" << request.getPort() << std::endl;
        std::cout << "contentLength=" << request.getContentLength() << std::endl;
        std::cout << "contentType=" << request.getContentType() << std::endl;
        std::cout << "bodyLength=" << request.getBody().length() << std::endl;
        std::cout << "isComplete=" << (request.isComplete() ? "true" : "false") << std::endl;
        std::cout << "hasBody=" << (request.hasBody() ? "true" : "false") << std::endl;
    }

    return parseResult ? 0 : 1;
}
