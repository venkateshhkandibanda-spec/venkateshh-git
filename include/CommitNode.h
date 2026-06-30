#pragma once
#include <string>

// A single commit in our history. Each node knows about the next one,
// which is all a singly linked list needs.
struct CommitNode {
    std::string hash;          // 8-char unique id for this commit
    std::string message;       // commit message
    std::string timestamp;     // when it was made
    std::string snapshotPath;  // where the actual files are stored on disk
    CommitNode* next;

    CommitNode(const std::string& h, const std::string& msg,
               const std::string& time, const std::string& path)
        : hash(h), message(msg), timestamp(time), snapshotPath(path), next(nullptr) {}
};
