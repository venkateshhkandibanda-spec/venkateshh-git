# VCS — Git-Style Version Control System in C++

A lightweight Git-style version control system built from scratch in C++, using object-oriented design, a custom singly linked list for commit history, and `std::filesystem` for snapshot-based file tracking.

## Features

- `init` — initialize a new repository in the current directory
- `add <file>` — stage a file for the next commit
- `commit -m "message"` — commit all staged files, generating a unique 8-character hash
- `log` — view the full commit history
- `status` — see staged and untracked files
- `diff <hash1> [hash2]` — line-by-line diff between two commits, or a commit vs. the current working directory (uses an LCS-based diff algorithm)
- `revert <hash>` — restore the working directory to any previous commit

## How it works

- **Commit history** is stored in a custom singly linked list (`CommitList`), with a tail pointer for O(1) commit insertion.
- **Each commit is a full snapshot**, not just a diff — every commit carries forward unchanged files from the previous commit and overlays whatever was newly staged. This makes revert and diff accurate without needing a separate "tree" object model.
- **Staging area** is a hidden `.vcs/staging` folder; `add` copies files there, `commit` copies them into a permanent snapshot folder under `.vcs/commits/<hash>/`.
- **Commit hashes** are generated from the commit message, timestamp, and commit count, then hex-encoded and truncated to 8 characters.
- **Diff algorithm** uses classic dynamic-programming LCS (Longest Common Subsequence) to compute minimal line-level insertions/deletions, the same core idea real diff tools use.
- **Persistence** across runs is handled by a plain-text `.vcs/log.txt` file; on startup, the repository reconstructs its in-memory linked list from this log.

## Folder structure

```
VCS-Project/
├── include/
│   ├── CommitNode.h     # single commit data + next pointer
│   ├── CommitList.h     # singly linked list of commits
│   └── Repository.h     # main class: staging, commits, diff, revert
├── src/
│   ├── CommitList.cpp
│   ├── Repository.cpp
│   └── main.cpp         # CLI entry point
└── README.md
```

## Build

```bash
g++ -std=c++17 -Iinclude src/*.cpp -o vcs
```

## Usage

```bash
./vcs init
./vcs add file.txt
./vcs commit -m "Initial commit"
./vcs log
./vcs status
./vcs diff <hash1> <hash2>
./vcs revert <hash>
```

## Design notes / known limitations

- Hashing is `std::hash`-based for simplicity, not cryptographic (no SHA1). This keeps the project dependency-free while still producing unique, Git-style short hashes.
- Diff is line-based (LCS), not byte-based — fine for source/text files, not intended for binary diffing.
- No branching or merging — history is strictly linear, matching the scope of this project.

## Tech used

C++17, OOP (encapsulation, constructor overloading, modular class design), custom singly linked list, `std::filesystem`, dynamic programming (LCS diff).
