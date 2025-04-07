#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <winsock2.h>
#include <Windows.h>

namespace fs = std::filesystem;

#pragma comment(lib, "ws2_32.lib")

void editFile(SOCKET clientSocket, const std::string& filename) {
    std::cout << "Editing file '" << filename << "'. Enter new content:\n";
    std::string newContent;
    std::getline(std::cin, newContent);

    std::string request = "EDIT " + filename;
    send(clientSocket, request.c_str(), request.size(), 0);

    // Send edited content to server
    send(clientSocket, newContent.c_str(), newContent.size(), 0);

    // Wait for confirmation from server
    char buffer[1024] = { 0 };
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Receive failed\n";
        return;
    }

    std::cout << "File edited successfully.\n";

    return;
}


void printAvailableFiles(SOCKET clientSocket) {
    // Send special command to server to request file list
    const char* listRequest = "LIST";
    //Sleep(1);
    send(clientSocket, listRequest, strlen(listRequest), 0);

    // Receive and print file list from server
    char buffer[10240] = { 0 };
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Receive failed\n";
        return;
    }

    std::cout << "\nAvailable files on server:\n" << buffer << std::endl;

    return;
}

void deleteFileOnServer(SOCKET clientSocket, const std::string& filename) {
    // Send DELETE request to the server
    std::string request = "DELETE " + filename;
    send(clientSocket, request.c_str(), request.size(), 0);

    // Receive response from the server
    char buffer[1024] = { 0 };
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Receive failed\n";
        return;
    }

    std::string response(buffer);

    // Print the response received from the server
    std::cout << "Server response: " << response << std::endl;
}

void createFile(SOCKET clientSocket, const std::string& filename) {
    std::string request = "CREATE " + filename;
    send(clientSocket, request.c_str(), request.size(), 0);
    char buffer[1024] = { 0 };
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Receive failed\n";
        return;
    }

    std::string response(buffer);

    // Print the response received from the server
    std::cout << "Server response: " << response << std::endl;

    return;
}

void deleteFile(const std::string& filename) {
    if (fs::exists(filename)) {
        if (fs::remove(filename)) {
            std::cout << "TempFile deleted successfully." << std::endl;
        }
        else {
            std::cerr << "Error: Unable to delete the file." << std::endl;
        }
    }
    else {
        std::cerr << "Error: File does not exist." << std::endl;
    }
}

void openFileInNotepad(const std::string& content) {
    // Create a temporary file
    std::ofstream tempFile("temp.txt");
    tempFile << content;
    tempFile.close();

    // Open the temporary file in Notepad
    system("notepad temp.txt");
}


int userLogin(SOCKET clientSocket,std::string user,std::string pass)
{   
    std::string request;

    request = "LOG " + user + ":" + pass;

    send(clientSocket, request.c_str(), request.size(), 0);

    char buffer[10240] = { 0 };
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Receive failed\n";
        int errorCode = WSAGetLastError();
        std::cerr << "Receive failed with error code: " << errorCode << std::endl;
        return -1;
    }

    std::string response(buffer);

    if (response == "Login successful")
        return 1;
    else
        return 0;

    return -1;
}

int main() {
    setvbuf(stdin, NULL, _IONBF, 0);
    WSADATA wsaData;
    SOCKET clientSocket;
    SOCKADDR_IN serverAddr;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    // Connect to server
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address
    serverAddr.sin_port = htons(12345); // Server port number
    if (connect(clientSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server\n";
    int checkLogin = 0;

    while (checkLogin != 1)
    {
        std::string user, pass;
        std::cout << "\nEnter username:\n";
        std::getline(std::cin, user);
        std::cout << "\nEnter password:\n";
        std::getline(std::cin, pass);
        checkLogin = userLogin(clientSocket,user,pass);

        if (checkLogin != 1)
            std::cout << "\nInvalid credentials. Try again";
    }

    while (true) {
        
        printAvailableFiles(clientSocket);
        // Request file name from user
        std::cout << "Enter the name of the file you want to open, edit/create/delete <filename> or 'exit' :\n";
        std::string filename;
        std::getline(std::cin, filename);

        if (filename == "exit")
        {
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }
        
        if (filename.substr(0, 5) == "edit ")
        {
            editFile(clientSocket, filename.substr(5));
            continue;
        }

        if (filename.substr(0, 7) == "delete ")
        {
            deleteFileOnServer(clientSocket, filename.substr(7));
            continue;
        }

        if (filename.substr(0, 7) == "create ")
        {
            createFile(clientSocket, filename.substr(7));
            continue;
        }
        // Send file name to server
        send(clientSocket, filename.c_str(), filename.size(), 0);

        // Receive file content from server
        char buffer[10240] = { 0 };
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "Receive failed\n";
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }

        std::string fileContent(buffer);

        // Open the file content in Notepad
        openFileInNotepad(fileContent);
        deleteFile("temp.txt");
    }  
    // Close connection to server
    return 0;
}
