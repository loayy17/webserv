#include <cstdlib>
#include <iostream>
#include "server/Client.hpp"
#include "server/Server.hpp"
#include "utils/Logger.hpp"

int main(int argc, char** argv) {
    uint16_t port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
        port == 0 && (port = 8080);
    }

    Logger::info("Starting WebServ...");
    Server server(port);

    if (!server.init()) {
        Logger::error("Failed to initialize server");
        return 1;
    }

    Logger::info("Waiting for connection...");

    int client_fd = server.acceptConnection();
    if (client_fd < 0) {
        Logger::error("Failed to accept connection");
        return 1;
    }

    Logger::info("Client connected!");
    Client client(client_fd);

    client.receiveData();
    std::string request = client.getBuffer();
    std::cout << "\n========== REQUEST ================================\n"
              << request << "\n====================================================\n"
              << std::endl;

    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Hello, World!";

    std::cout << "\n========== RESPONSE ===============================\n"
              << response << "\n====================================================\n"
              << std::endl;

    client.sendData(response);
    Logger::info("Response sent");

    client.closeConnection();
    server.stop();
    Logger::info("Server stopped");

    return 0;
}
