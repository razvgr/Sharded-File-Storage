#include "Server.h"

std::string listFiles(const std::string& directory) {
    std::string fileList;
    for (const auto& entry : fs::directory_iterator(directory)) {
        fileList += entry.path().filename().string() + "\n";
    }
    return fileList;
}

std::string Server::readFile(const std::string& filename) {
    std::vector<std::string> shardNames = fm.generateShardNames(filename, 2);

    std::string content = fm.reuniteShardsandRead(shardNames);

    return content;
}

void Server::handleList(SOCKET clientSocket)
{
    std::string fileList = fm.getFileList();
    send(clientSocket, fileList.c_str(), fileList.size(), 0);

    return;
}

void Server::handleEdit(SOCKET clientSocket, std::string& filename)
{
    char buffer[1024] = { 0 };
    std::string editedContent;

    int bytesReceived;
    do {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0)
            editedContent.append(buffer, bytesReceived);
        else if (bytesReceived == 0)
            break;
        else {
            std::cerr << "Receive failed\n";
            closesocket(clientSocket);
            return;
        }
    } while (bytesReceived == sizeof(buffer));

    if (fm.editFile(filename, editedContent))
    {
        std::cout << "File '" << filename << "' edited and saved successfully.\n";
        last_event = "File '" + filename + "' edited and saved successfully.\n";
        const char* confirmationMessage = "File edited and divided successfully";
        send(clientSocket, confirmationMessage, strlen(confirmationMessage), 0);
    }
    else
    {
        const char* confirmationMessage = "File editing error";
        last_event = confirmationMessage;
        send(clientSocket, confirmationMessage, strlen(confirmationMessage), 0);
        
    }

    return;
}

void Server::handleDelete(SOCKET clientSocket, const std::string& filename) {

    if (fm.deleteFile(filename))
    {
        send(clientSocket, "File deleted successfully", 26, 0);
        std::cout << "File '" << filename << "' deleted successfully.\n";

        last_event = "File '" + filename + "' deleted successfully.\n";
    }
    else
    {
        send(clientSocket, "Failed to delete file", 21, 0);
        last_event = "Delete failed\n";
    }

}

void Server::handleCreate(SOCKET clientSocket, const std::string& filename) {
    fm.createFile(filename);
    std::string confirmation = "File '" + filename + "' created successfully.";
    send(clientSocket, confirmation.c_str(), confirmation.size(), 0);

    std::cout << "File '" << filename << "' created successfully.\n";
    last_event = confirmation;
}

//void Server::handleAskEvent(SOCKET clientSocket)
//{
//    send(clientSocket, last_event.c_str(), last_event.size(), 0);
//}


void handleLogin(SOCKET clientSocket, std::string request, Database &database) {
    std::wstring wideUsername = L"";
    std::wstring widePassword = L"";

    std::string credentials = request.substr(4);
    std::string delimiter = ":";
    std::string username = credentials.substr(0, credentials.find(delimiter));
    std::string password = credentials.substr(credentials.find(delimiter) + 1);

    wideUsername.assign(username.begin(), username.end());
    widePassword.assign(password.begin(), password.end());

    SQLRETURN retcode;
    SQLHSTMT hStmt = NULL;
    SQLWCHAR sqlState[6];
    SQLINTEGER nativeError;
    SQLWCHAR errMsg[SQL_MAX_MESSAGE_LENGTH];
    SQLSMALLINT errMsgLength;

    std::wstring query = L"SELECT * FROM users WHERE username = ? AND password = ?";
    retcode = SQLAllocHandle(SQL_HANDLE_STMT,database.hDbc, &hStmt);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to allocate statement handle\n";
        return;
    }

    retcode = SQLPrepare(hStmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to prepare SQL statement\n";
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    retcode = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 51, 0, (SQLPOINTER)wideUsername.c_str(), wideUsername.size() * sizeof(wchar_t), NULL);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to bind username parameter\n";
        SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, errMsg, sizeof(errMsg), &errMsgLength);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    retcode = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 51, 0, (SQLPOINTER)widePassword.c_str(), widePassword.size() * sizeof(wchar_t), NULL);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to bind password parameter\n";
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    retcode = SQLExecute(hStmt);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to execute SQL statement\n";
        SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, errMsg, sizeof(errMsg), &errMsgLength);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    retcode = SQLFetch(hStmt);
    if (retcode == SQL_NO_DATA) {
        std::cerr << "No rows returned\n";
        const char* response = "Invalid credentials. Please try again.";
        send(clientSocket, response, strlen(response), 0);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

        return;
    }
    else if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to fetch rows\n";
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    SQLLEN rowCount = 0;
    do {
        ++rowCount;
    } while (SQLFetch(hStmt) == SQL_SUCCESS);

    const char* response = (rowCount == 1) ? "Login successful" : "Invalid credentials. Please try again.";
    send(clientSocket, response, strlen(response), 0);

}

bool Server::Initialize(int port) {

    last_event = "empty\n";

    fm = FileManager::getInstance(); 
    fm.initFilelist("C:\\Users\\razva\\Documents\\programe info\\servertest\\empty_files\\");
    fm.initServerAddresses();

    std::cout << "Connecting to database...\n";
    database.connect();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }

    std::cout << "Server listening on port " << port << "...\n";

    return true;
}

ClientConnection Server::AcceptConnections() {
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);


    SOCKET clientSocket = accept(serverSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed\n";
        exit(1);
    }

    std::cout << "Client connected\n";

    ClientConnection client_Connection(clientSocket);

    return client_Connection;

}

Server::~Server() {
    closesocket(serverSocket);
    WSACleanup();
}

void Server::handleReq(SOCKET clientSocket) {
    char buffer[10240] = { 0 };
    
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesReceived == SOCKET_ERROR) {
        closesocket(clientSocket);
        return;
    }
    else if (bytesReceived == 0) {
        std::cout << "Client disconnected\n";
        closesocket(clientSocket);
        return;
    }

    std::string request(buffer);

    if (request == "LIST") {
        std::cout << "List requested\n";
        handleList(clientSocket);
        return;
    }
    else if (request.substr(0, 4) == "LOG ")
    {
        std::cout << request << std::endl;
        handleLogin(clientSocket, request,database);
        return;
    }
    else if (request.substr(0, 5) == "EDIT ") {
        std::string filename = request.substr(5);

        handleEdit(clientSocket, filename);
        return;
    }
    else if (request.substr(0, 7) == "CREATE ") {
        std::string filename = request.substr(7);
        handleCreate(clientSocket, filename);
        return;
    }
    else if (request.substr(0, 7) == "DELETE ") {
        std::string filename = request.substr(7);
        handleDelete(clientSocket, filename);
        return;
    }
    else
    {
        std::string filename(buffer);

        std::string fileContent = readFile(filename);

        if (fileContent.empty())
            fileContent = "Error: Empty file!\n";

        send(clientSocket, fileContent.c_str(), fileContent.size(), 0);
        return;
    }

    return;
}


bool isSocketOpen(SOCKET socket) {
    int error;
    int len = sizeof(error);
    int result = getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len);
    if (result == SOCKET_ERROR) {
        std::cerr << "Socket closed.\n";
        return false;
    }
    return (error == 0);
}

void Server::handleClient(const ClientConnection& client) {

    bool flag = 1;

    while (flag)
    {
        this->handleReq(client.getClientSocket());

        if (!isSocketOpen(client.getClientSocket()))
            flag = 0;
    }
}
