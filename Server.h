#pragma once
#include "ClientConnection.h"
#include"Database.h"
#include"FileManager.h"
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include<functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <sql.h>
#include <sqlext.h>
#include <string>
#include <thread>
#include <windows.h>
#include <winsock2.h>

namespace fs = std::filesystem;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "odbc32.lib")

class Server {
private:
    SOCKET serverSocket;
    FileManager fm;
    std::string last_event;

public:
    Database database;

    Server() {};

    ~Server();

    bool Initialize(int port);

    ClientConnection AcceptConnections();

    void handleReq(SOCKET clientSocket);
    
    void handleClient(const ClientConnection& client);

    std::string readFile(const std::string& filename);

    void handleList(SOCKET clientSocket);

    void handleEdit(SOCKET clientSocket, std::string& filename);

    void handleDelete(SOCKET clientSocket, const std::string& filename);

    void handleCreate(SOCKET clientSocket, const std::string& filename);

    //void handleAskEvent(SOCKET clientSocket);

};