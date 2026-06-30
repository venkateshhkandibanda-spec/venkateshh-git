#include "CommitList.h"
#include <iostream>

CommitList::CommitList() : head(nullptr), tail(nullptr), count(0) {}

CommitList::~CommitList() {
    // clean up every node manually since we're managing raw pointers ourselves
    CommitNode* current = head;
    while (current != nullptr) {
        CommitNode* toDelete = current;
        current = current->next;
        delete toDelete;
    }
}

void CommitList::addCommit(const std::string& hash, const std::string& message,
                            const std::string& timestamp, const std::string& snapshotPath) {
    CommitNode* node = new CommitNode(hash, message, timestamp, snapshotPath);

    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;  // O(1) tail insertion, no traversal needed
        tail = node;
    }
    count++;
}

CommitNode* CommitList::findByHash(const std::string& hash) const {
    CommitNode* current = head;
    while (current != nullptr) {
        if (current->hash == hash) return current;
        current = current->next;
    }
    return nullptr;
}

CommitNode* CommitList::getLast() const {
    return tail;  // O(1) thanks to the tail pointer we maintain
}

void CommitList::printLog() const {
    if (head == nullptr) {
        std::cout << "No commits yet. Use 'vcs add' and 'vcs commit' first.\n";
        return;
    }

    CommitNode* current = head;
    while (current != nullptr) {
        std::cout << "commit " << current->hash << "\n";
        std::cout << "Date:   " << current->timestamp << "\n";
        std::cout << "        " << current->message << "\n\n";
        current = current->next;
    }
}

int CommitList::size() const { return count; }
