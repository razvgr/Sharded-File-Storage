#include "FileManager.h"

FileManager* FileManager::instance = nullptr;

FileManager& FileManager::getInstance()
{
    if (!FileManager::instance)
        FileManager::instance = new FileManager();

    return (*FileManager::instance);
}

std::string FileManager::getFileList()
{
    return real_fileList;
}

std::string FileManager::listDirectory(const std::string& id) {
    std::stringstream ss(this->real_fileList);
    std::string token;
    std::string filteredString;

    while (ss >> token) {
        size_t idPos = token.find(id);
        if (idPos != std::string::npos) {
            filteredString += token.substr(idPos + id.length());
            if (!filteredString.empty() && filteredString[0] == '/') {
                filteredString.erase(0, 1);
            }
            filteredString += " ";
        }
    }

    if (!filteredString.empty()) {
        filteredString.pop_back();
    }

    std::istringstream iss(filteredString);
    std::ostringstream oss;
    std::string word;

    bool isFirstWord = true;

    while (iss >> word) {
        if (!isFirstWord) {
            oss << " "; 
        }
        else {
            isFirstWord = false;
        }

        if (!word.empty()) {
            if (word[0] == '/') {
                word = word.substr(1); 
            }
            if (word.size() >= 4 && std::isdigit(word[0]) && std::isdigit(word[1]) && std::isdigit(word[2]) && word[3] == '/') {
                word = word.substr(0, 3);
            }
            oss << word;
        }
    }

    std::istringstream new_iss(oss.str());
    std::unordered_set<std::string> uniqueWords;

    std::string result;

    while (new_iss >> word) {
        if (uniqueWords.find(word) == uniqueWords.end()) {
            result += word + " ";
            uniqueWords.insert(word);
        }
    }

    if (!result.empty())
        result.pop_back();

    return result;

}



void FileManager::divideFileIntoShards(const std::string& filename, const std::vector<std::string>& shardNames) {
    std::ifstream inputFile(filename, std::ios::binary | std::ios::ate);

    if (!inputFile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    std::streampos fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    std::streamsize shardSize = static_cast<std::streamsize>(fileSize / storageServerAddresses.size());
    std::streamsize lastShardSize = shardSize + static_cast<std::streamsize>(fileSize % storageServerAddresses.size());

    for (size_t i = 0; i < storageServerAddresses.size(); ++i) {
        std::streamsize currentShardSize = (i == storageServerAddresses.size() - 1) ? lastShardSize : shardSize;

        std::vector<char> buffer(currentShardSize);
        inputFile.read(buffer.data(), currentShardSize);

        sendShardToStorageServer(storageServerAddresses[i], shardNames[i], std::string(buffer.begin(), buffer.end()));
    }

    inputFile.close();
}


bool FileManager::sendShardToStorageServer(const std::string& serverAddress, const std::string& shardName, const std::string& content) {
    //parsez ip server storage si port
    size_t colonPos = serverAddress.find(':');
    std::string ip = serverAddress.substr(0, colonPos);
    int port = std::stoi(serverAddress.substr(colonPos + 1));

    //creez socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return 0;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddr.sin_port = htons(port);

    //conecteaza la server
    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection to storage server failed\n";
        closesocket(sock);
        return 0;
    }

    std::string message = "STORE " + shardName + "\n" + content;
    send(sock, message.c_str(), message.size(), 0);

    closesocket(sock);
    return 1;
}

std::string FileManager::receiveShardFromStorageServer(const std::string& shardName, const std::string& serverAddress) {
    
    size_t colonPos = serverAddress.find(':');
    std::string ip = serverAddress.substr(0, colonPos);
    int port = std::stoi(serverAddress.substr(colonPos + 1));

    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return "";
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddr.sin_port = htons(port);

    
    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection to storage server failed\n";
        closesocket(sock);
        return "";
    }
    else
        std::cout << "Connected to storage server\n";

    
    std::string message = "GET " + shardName;
    send(sock, message.c_str(), message.size(), 0);

    // primesc shardul
    char buffer[1024];
    int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Receive failed\n";
        closesocket(sock);
        return "";
    }

    std::string content(buffer, bytesReceived);
    closesocket(sock);
    return content;
}


std::string FileManager::reuniteShardsandRead(const std::vector<std::string>& shardNames) {
    std::string reconstructedFile;

    for (size_t i = 0; i < shardNames.size(); ++i) {
        std::string shardContent = receiveShardFromStorageServer(shardNames[i], storageServerAddresses[i]);
        reconstructedFile += shardContent;
    }

    return reconstructedFile;
}

std::string addAndSortWords(const std::string& inputString, const std::string& wordToAdd) {
    
    std::istringstream iss(inputString);
    std::vector<std::string> words;
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }

    words.push_back(wordToAdd);

    std::sort(words.begin(), words.end());

    std::string result;
    for (const auto& w : words) {
        result += w + " ";
    }
    
    if (!result.empty()) {
        result.pop_back();
    }
    return result;
}


std::string deleteAndSortWords(const std::string& inputString, const std::string& wordToDelete) {

    std::istringstream iss(inputString);
    std::vector<std::string> words;
    std::string word;
    while (iss >> word) {
        
        if (word != wordToDelete) {
            words.push_back(word);
        }
    }


    std::sort(words.begin(), words.end());

    std::string result;
    for (const auto& w : words) {
        result += w + " "; 
    }

    if (!result.empty()) {
        result.pop_back();
    }
    else
        result = " ";

    return result;
}

std::vector<std::string> FileManager::generateShardNames(const std::string& originalFilename, int numShards) {
    std::vector<std::string> shardNames;
    std::string prefix = originalFilename.substr(0, originalFilename.find_last_of('.'));
    std::string extension = originalFilename.substr(originalFilename.find_last_of('.'));
    for (int i = 0; i < numShards; ++i) {
        std::ostringstream oss;
        oss << prefix << "_shard" << i << extension;
        shardNames.push_back(oss.str());
    }
    return shardNames;
}

std::string recreateOriginalFilename(const std::vector<std::string>& shardNames) {
    if (shardNames.empty()) {
        return "";
    }

    size_t pos = shardNames[0].find("_shard");
    std::string prefix = shardNames[0].substr(0, pos);

    std::string extension = shardNames[0].substr(shardNames[0].find_last_of('.'));
    return prefix + extension;
}

void FileManager::initFilelist(const std::string& directory) {
    
    real_fileList = "\n";
    for (const auto& entry : fs::directory_iterator(directory)) {
        this->real_fileList += entry.path().filename().string() + "\n";
    }
    

    return;
}

void FileManager::initServerAddresses()
{
    storageServerAddresses = { "127.0.0.1:12346","127.0.0.1:12347" };
}

 bool FileManager::editFile(const std::string& filename, const std::string& content) {

     if (!deleteFile(filename))
     {
         std::cout<<"File was not divided yet!\n";
     }

    createFile(filename);

    std::ofstream file("C:\\Users\\razva\\Documents\\programe info\\servertest\\" + filename);
    file << content;
    file.close();

    std::vector<std::string> shard_names = generateShardNames(filename, 2);
    divideFileIntoShards(filename, shard_names);

    return true;
}

 bool FileManager::deleteFile(const std::string& filename) {
     
     bool ok = 1, ok1 = 1;

     std::vector<std::string> shardNames = generateShardNames(filename, 2);
     std::string shardPath_1 = "C:\\Users\\razva\\Documents\\programe info\\StorageServer1\\" + shardNames[0];
     std::string shardPath_2 = "C:\\Users\\razva\\Documents\\programe info\\StorageServer2\\" + shardNames[1];
     std::string junkFilePath = "C:\\Users\\razva\\Documents\\programe info\\servertest\\" + filename;
     std::string junkFilePathOther = "C:\\Users\\razva\\Documents\\programe info\\servertest\\empty_files\\" + filename;
     if (std::remove(shardPath_1.c_str()) != 0 || std::remove(shardPath_2.c_str()) != 0  || std::remove(junkFilePath.c_str()) != 0)
         ok=0;

     if (std::remove(junkFilePathOther.c_str()) != 0)
         ok1 = 0;

     this->real_fileList = deleteAndSortWords(this->real_fileList, filename);

     return ok||ok1;
}

void FileManager::createFile(const std::string& filename) {
    std::ofstream file("C:\\Users\\razva\\Documents\\programe info\\servertest\\empty_files\\" + filename);
    file <<" ";
    file.close();

    this->real_fileList = addAndSortWords(this->real_fileList, filename);

}


/*
create file = creeaza fisiere nedivizate in empty_files

delete file = iau shardnames, trimit shard1 la storage1... trimit request la servere sa stearga shardul + delete de la empty

edit file = delete file + create file cu content nou (acelasi nume cu originalu), divizez in 2 sharduri, trimit fiecare shard la storageul lui

view file = reunesc shardurile si deschid
*/

