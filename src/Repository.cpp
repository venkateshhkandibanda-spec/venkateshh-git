#include "Repository.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>
#include <functional>
#include <algorithm>

namespace fs = std::filesystem;

Repository::Repository() : Repository(".") {}

Repository::Repository(const std::string& path) {
    rootPath = path;
    vcsPath = rootPath + "/.vcs";
    stagingPath = vcsPath + "/staging";
    commitsPath = vcsPath + "/commits";
    logFilePath = vcsPath + "/log.txt";

    // if a repo already exists here, rebuild the linked list from disk
    if (isInitialized()) {
        loadHistoryFromDisk();
    }
}

bool Repository::isInitialized() {
    return fs::exists(vcsPath);
}

std::string Repository::currentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::string t = std::asctime(std::localtime(&now));
    if (!t.empty() && t.back() == '\n') t.pop_back();  // asctime adds a trailing newline
    return t;
}

std::string Repository::generateHash(const std::string& message, const std::string& timestamp) {
    // not cryptographic, but good enough to produce a stable-looking 8 char id
    std::string raw = message + timestamp + std::to_string(commitHistory.size());
    size_t hashed = std::hash<std::string>{}(raw);

    std::stringstream ss;
    ss << std::hex << hashed;
    std::string hex = ss.str();

    if (hex.length() < 8) hex = std::string(8 - hex.length(), '0') + hex;
    return hex.substr(0, 8);
}

void Repository::init() {
    if (isInitialized()) {
        std::cout << "Repository already initialized in " << rootPath << "\n";
        return;
    }

    fs::create_directories(stagingPath);
    fs::create_directories(commitsPath);
    std::ofstream logFile(logFilePath);  // just create an empty log file
    logFile.close();

    std::cout << "Initialized empty VCS repository in " << fs::absolute(vcsPath) << "\n";
}

void Repository::add(const std::string& filename) {
    if (!isInitialized()) {
        std::cout << "Not a vcs repository. Run 'vcs init' first.\n";
        return;
    }

    fs::path source = fs::path(rootPath) / filename;
    if (!fs::exists(source)) {
        std::cout << "File not found: " << filename << "\n";
        return;
    }

    fs::path destination = fs::path(stagingPath) / filename;
    fs::copy_file(source, destination, fs::copy_options::overwrite_existing);

    std::cout << "Staged " << filename << " for commit\n";
}

void Repository::commit(const std::string& message) {
    if (!isInitialized()) {
        std::cout << "Not a vcs repository. Run 'vcs init' first.\n";
        return;
    }

    if (fs::is_empty(stagingPath)) {
        std::cout << "Nothing staged. Use 'vcs add <file>' before committing.\n";
        return;
    }

    std::string timestamp = currentTimestamp();
    std::string hash = generateHash(message, timestamp);
    fs::path commitDir = fs::path(commitsPath) / hash;
    fs::create_directories(commitDir);

    // carry forward every file from the previous commit's full snapshot first,
    // so each commit is a complete tree, not just whatever happened to be staged
    CommitNode* previous = commitHistory.getLast();
    if (previous != nullptr) {
        fs::path prevSnapshot(previous->snapshotPath);
        if (fs::exists(prevSnapshot)) {
            for (const auto& entry : fs::directory_iterator(prevSnapshot)) {
                fs::copy(entry.path(), commitDir / entry.path().filename(),
                          fs::copy_options::overwrite_existing | fs::copy_options::recursive);
            }
        }
    }

    // now overlay whatever is staged, overwriting any carried-forward versions
    int fileCount = 0;
    for (const auto& entry : fs::directory_iterator(stagingPath)) {
        fs::copy(entry.path(), commitDir / entry.path().filename(),
                  fs::copy_options::overwrite_existing | fs::copy_options::recursive);
        fileCount++;
    }

    commitHistory.addCommit(hash, message, timestamp, commitDir.string());
    appendToLogFile(hash, message, timestamp);

    // clear staging area now that the snapshot is safely stored
    fs::remove_all(stagingPath);
    fs::create_directories(stagingPath);

    std::cout << "[" << hash << "] " << message << "\n";
    std::cout << fileCount << " file(s) committed\n";
}

void Repository::appendToLogFile(const std::string& hash, const std::string& message, const std::string& timestamp) {
    std::ofstream logFile(logFilePath, std::ios::app);
    logFile << hash << "|" << timestamp << "|" << message << "\n";
}

void Repository::loadHistoryFromDisk() {
    std::ifstream logFile(logFilePath);
    if (!logFile.is_open()) return;

    std::string line;
    while (std::getline(logFile, line)) {
        if (line.empty()) continue;

        size_t firstBar = line.find('|');
        size_t secondBar = line.find('|', firstBar + 1);
        if (firstBar == std::string::npos || secondBar == std::string::npos) continue;

        std::string hash = line.substr(0, firstBar);
        std::string timestamp = line.substr(firstBar + 1, secondBar - firstBar - 1);
        std::string message = line.substr(secondBar + 1);
        std::string snapshotPath = (fs::path(commitsPath) / hash).string();

        commitHistory.addCommit(hash, message, timestamp, snapshotPath);
    }
}

void Repository::log() {
    if (!isInitialized()) {
        std::cout << "Not a vcs repository. Run 'vcs init' first.\n";
        return;
    }
    commitHistory.printLog();
}

void Repository::revert(const std::string& hash) {
    if (!isInitialized()) {
        std::cout << "Not a vcs repository. Run 'vcs init' first.\n";
        return;
    }

    CommitNode* target = commitHistory.findByHash(hash);
    if (target == nullptr) {
        std::cout << "No commit found with hash " << hash << "\n";
        return;
    }

    fs::path snapshotDir(target->snapshotPath);
    if (!fs::exists(snapshotDir)) {
        std::cout << "Snapshot data missing for commit " << hash << "\n";
        return;
    }

    int restoredCount = 0;
    for (const auto& entry : fs::directory_iterator(snapshotDir)) {
        fs::path destination = fs::path(rootPath) / entry.path().filename();
        fs::copy(entry.path(), destination,
                  fs::copy_options::overwrite_existing | fs::copy_options::recursive);
        restoredCount++;
    }

    std::cout << "Reverted working directory to commit " << hash
              << " (" << restoredCount << " file(s) restored)\n";
}

std::vector<std::string> Repository::readLines(const std::string& filepath) {
    std::vector<std::string> lines;
    std::ifstream file(filepath);
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

void Repository::status() {
    if (!isInitialized()) {
        std::cout << "Not a vcs repository. Run 'vcs init' first.\n";
        return;
    }

    std::cout << "Staged files (ready to commit):\n";
    bool anyStaged = false;
    for (const auto& entry : fs::directory_iterator(stagingPath)) {
        std::cout << "  " << entry.path().filename().string() << "\n";
        anyStaged = true;
    }
    if (!anyStaged) std::cout << "  (none)\n";

    // figure out what's in the working directory but neither staged nor in the last commit
    std::cout << "\nUntracked files:\n";
    bool anyUntracked = false;

    fs::path lastSnapshot;
    CommitNode* last = commitHistory.getLast();
    if (last != nullptr) {
        lastSnapshot = fs::path(last->snapshotPath);
    }

    for (const auto& entry : fs::directory_iterator(rootPath)) {
        if (entry.path().filename() == ".vcs") continue;
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        bool inStaging = fs::exists(fs::path(stagingPath) / filename);
        bool inLastCommit = !lastSnapshot.empty() && fs::exists(lastSnapshot / filename);

        if (!inStaging && !inLastCommit) {
            std::cout << "  " << filename << "\n";
            anyUntracked = true;
        }
    }
    if (!anyUntracked) std::cout << "  (none)\n";
}

// Classic LCS table approach: build the table, then walk it backwards
// to figure out which lines were kept, removed, or added. This is the
// same idea real diff tools use to avoid printing a removal+addition
// for every line when only one line in the middle actually changed.
void Repository::printLCSDiff(const std::vector<std::string>& linesA, const std::vector<std::string>& linesB) {
    size_t n = linesA.size();
    size_t m = linesB.size();

    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

    for (size_t i = 1; i <= n; i++) {
        for (size_t j = 1; j <= m; j++) {
            if (linesA[i - 1] == linesB[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    // backtrack from the bottom-right corner, building the diff in reverse
    std::vector<std::string> output;
    size_t i = n, j = m;
    bool anyChange = false;

    while (i > 0 && j > 0) {
        if (linesA[i - 1] == linesB[j - 1]) {
            i--; j--;  // unchanged line, just step diagonally, nothing to print
        } else if (dp[i - 1][j] >= dp[i][j - 1]) {
            output.push_back("  - " + linesA[i - 1]);  // line only in A -> removed
            anyChange = true;
            i--;
        } else {
            output.push_back("  + " + linesB[j - 1]);  // line only in B -> added
            anyChange = true;
            j--;
        }
    }
    while (i > 0) { output.push_back("  - " + linesA[i - 1]); anyChange = true; i--; }
    while (j > 0) { output.push_back("  + " + linesB[j - 1]); anyChange = true; j--; }

    if (!anyChange) {
        std::cout << "  (no changes)\n";
        return;
    }

    // we built it back-to-front, so reverse before printing
    for (auto it = output.rbegin(); it != output.rend(); ++it) {
        std::cout << *it << "\n";
    }
}

void Repository::diff(const std::string& hash1, const std::string& hash2) {
    if (!isInitialized()) {
        std::cout << "Not a vcs repository. Run 'vcs init' first.\n";
        return;
    }

    CommitNode* commitA = commitHistory.findByHash(hash1);
    if (commitA == nullptr) {
        std::cout << "No commit found with hash " << hash1 << "\n";
        return;
    }

    fs::path dirA(commitA->snapshotPath);
    fs::path dirB;
    bool compareWithWorkingDir = (hash2.empty());

    if (!compareWithWorkingDir) {
        CommitNode* commitB = commitHistory.findByHash(hash2);
        if (commitB == nullptr) {
            std::cout << "No commit found with hash " << hash2 << "\n";
            return;
        }
        dirB = fs::path(commitB->snapshotPath);
    } else {
        dirB = fs::path(rootPath);
    }

    std::cout << "Comparing " << hash1 << " -> "
              << (compareWithWorkingDir ? "working directory" : hash2) << "\n\n";

    // compare every file that exists in commit A against the same filename in B
    for (const auto& entry : fs::directory_iterator(dirA)) {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        fs::path fileB = dirB / filename;

        std::cout << "--- " << filename << " ---\n";

        if (!fs::exists(fileB)) {
            std::cout << "  (file removed)\n\n";
            continue;
        }

        std::vector<std::string> linesA = readLines(entry.path().string());
        std::vector<std::string> linesB = readLines(fileB.string());

        printLCSDiff(linesA, linesB);
        std::cout << "\n";
    }
}
