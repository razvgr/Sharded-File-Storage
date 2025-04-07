#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NUM_CLIENTS 20
#define PORT 12345
#include "Server.h"


int main() {
    Server server;
    if (!server.Initialize(PORT)) {
        std::cerr << "Server initialization failed\n";
        return 1;
    }

    std::vector<std::thread> clientThreads;

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        ClientConnection clientconn = server.AcceptConnections();

        clientThreads.emplace_back([&server, clientconn = std::move(clientconn)]() { server.handleClient( std::move(clientconn)); });

    }

    for (auto& thread : clientThreads) 
    {
        thread.join();
    }

    return 0;
}