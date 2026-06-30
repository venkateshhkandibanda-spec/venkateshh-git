#pragma once
#include "CommitList.h"
#include <string>
#include <vector>

// Repository is the main class that ties everything together.
// It owns a CommitList for history and talks to the filesystem
// for staging and snapshotting files.
class Repository {
private:
    std::string rootPath;
    std::string vcsPath;
    std::string stagingPath;
    std::string commitsPath;
    std::string logFilePath;
    CommitList commitHistory;

    std::string generateHash(const std::string& message, const std::string& timestamp);
    std::string currentTimestamp();
    void loadHistoryFromDisk();
    void appendToLogFile(const std::string& hash, const std::string& message, const std::string& timestamp);
    bool isInitialized();
    std::vector<std::string> readLines(const std::string& filepath);
    void printLCSDiff(const std::vector<std::string>& linesA, const std::vector<std::string>& linesB);

public:
    // constructor overloading: default to current directory, or pass a custom path
    Repository();
    Repository(const std::string& path);

    void init();
    void add(const std::string& filename);
    void commit(const std::string& message);
    void log();
    void revert(const std::string& hash);
    void status();
    void diff(const std::string& hash1, const std::string& hash2);
};
