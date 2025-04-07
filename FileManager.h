#pragma once
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include<unordered_set>
#include <vector>
#include <windows.h>

namespace fs = std::filesystem;

class FileManager {

private:
    FileManager() {}
    static FileManager* instance;
    ~FileManager(){}
    FileManager(const FileManager&) = delete;

    std::string real_fileList;
    std::vector<std::string> storageServerAddresses;

    friend class Server;

public:
    static FileManager& getInstance();

    void initFilelist(const std::string& directory);

    void initServerAddresses();

    bool editFile(const std::string& filename, const std::string& content);

    bool deleteFile(const std::string& filename);

    std::string receiveShardFromStorageServer(const std::string& shardFilename, const std::string& serverAddress);

    void createFile(const std::string& filename);

    bool sendShardToStorageServer(const std::string& serverAddress, const std::string& shardName, const std::string& content);

    void divideFileIntoShards(const std::string& filename, const std::vector<std::string>& shardNames);

    std::string reuniteShardsandRead(const std::vector<std::string>& shardNames);

    std::string getFileList();

    std::string listDirectory(const std::string& id);

    std::vector<std::string> generateShardNames(const std::string& originalFilename, int numShards);
};
