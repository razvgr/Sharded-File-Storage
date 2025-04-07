#include <iostream>
#include <fstream>
#include <string>
#include <WinSock2.h>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;
#pragma comment(lib, "ws2_32.lib")

void handleClient(SOCKET clientSocket) {
    char buffer[1024] = { 0 };
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Receive failed\n";
        closesocket(clientSocket);
        return;
    }

    std::string request(buffer);
    if (request.substr(0, 5) == "STORE") {
        size_t pos = request.find("\n");
        std::string shardName = request.substr(6, pos - 6);
        std::string content = request.substr(pos + 1);

        std::ofstream outputFile(shardName, std::ios::binary);
        outputFile.write(content.c_str(), content.size());
        outputFile.close();
    }
    else if (request.substr(0, 3) == "GET") {
        std::string shardName = request.substr(4);
        std::ifstream inputFile(shardName, std::ios::binary);

        if (inputFile.is_open()) {
            std::string content((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
            send(clientSocket, content.c_str(), content.size(), 0);
            inputFile.close();
        }
        else {
            std::string response = "ERROR\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        }
    }

    closesocket(clientSocket);
}

int main() {
    
    int port = 12346;

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "StorageServer 1 listening on port " << port << "\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket != INVALID_SOCKET) {
            std::thread clientThread(handleClient, clientSocket);
            clientThread.detach();
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
