#include "Repository.h"
#include <iostream>
#include <string>

// Simple CLI dispatcher: vcs init / vcs add <file> / vcs commit -m "msg" / vcs log / vcs revert <hash>
// / vcs status / vcs diff <hash1> [hash2]
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: vcs <command> [args]\n";
        std::cout << "Commands: init, add <file>, commit -m \"message\", log, revert <hash>, status, diff <hash1> [hash2]\n";
        return 0;
    }

    Repository repo(".");
    std::string command = argv[1];

    if (command == "init") {
        repo.init();
    }
    else if (command == "add") {
        if (argc < 3) {
            std::cout << "Usage: vcs add <filename>\n";
            return 1;
        }
        repo.add(argv[2]);
    }
    else if (command == "commit") {
        if (argc < 4 || std::string(argv[2]) != "-m") {
            std::cout << "Usage: vcs commit -m \"your message\"\n";
            return 1;
        }
        repo.commit(argv[3]);
    }
    else if (command == "log") {
        repo.log();
    }
    else if (command == "revert") {
        if (argc < 3) {
            std::cout << "Usage: vcs revert <hash>\n";
            return 1;
        }
        repo.revert(argv[2]);
    }
    else if (command == "status") {
        repo.status();
    }
    else if (command == "diff") {
        if (argc < 3) {
            std::cout << "Usage: vcs diff <hash1> [hash2]\n";
            return 1;
        }
        std::string hash2 = (argc >= 4) ? argv[3] : "";
        repo.diff(argv[2], hash2);
    }
    else {
        std::cout << "Unknown command: " << command << "\n";
    }

    return 0;
}
