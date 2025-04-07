#pragma once
#include <iostream>
#include <string>
#include <filesystem>
#include <winsock2.h>
#include <windows.h>
#include <fstream>

namespace fs = std::filesystem;

#pragma comment(lib, "ws2_32.lib")

class ClientConnection {
private:
    SOCKET clientSocket;

public:
    ClientConnection(const ClientConnection& other) : clientSocket(other.clientSocket) {}

    ClientConnection(SOCKET socket) : clientSocket(socket) {}

    int Receive(char* buffer, int bufferSize);

    int Send(const char* data, int dataSize);

    ~ClientConnection();

    SOCKET getClientSocket()const;
};