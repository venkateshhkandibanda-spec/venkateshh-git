#pragma once
#include "CommitNode.h"
#include <string>

// Custom singly linked list that holds the project's commit history.
// We keep a tail pointer around so adding a new commit is O(1)
// instead of walking the whole list every time.
class CommitList {
private:
    CommitNode* head;
    CommitNode* tail;
    int count;

public:
    CommitList();
    ~CommitList();

    void addCommit(const std::string& hash, const std::string& message,
                    const std::string& timestamp, const std::string& snapshotPath);
    CommitNode* findByHash(const std::string& hash) const;
    CommitNode* getLast() const;
    void printLog() const;
    int size() const;
};
