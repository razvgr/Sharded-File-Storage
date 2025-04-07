#include "ClientConnection.h"

int ClientConnection::Receive(char* buffer, int bufferSize) {
    return recv(clientSocket, buffer, bufferSize, 0);
}

int ClientConnection::Send(const char* data, int dataSize) {
    return send(clientSocket, data, dataSize, 0);
}

ClientConnection::~ClientConnection() {
    //closesocket(clientSocket);
}

SOCKET ClientConnection::getClientSocket()const
{
    return clientSocket;
}