
# **Ctrl + Z - Version Control System**

## **Table of Contents**

- [Description](#description)
- [Objective](#objective)
- [Features](#features)
- [How to Use](#how-to-use)
- [Project Structure](#project-structure)
- [Technologies Used](#technologies-used)
- [Contributors](#contributors)
- [Future Enhancements](#future-enhancements)

---

## **Description**

**Ctrl + Z** is a custom version control system developed as a final project for the Operating Systems course. It allows users to manage repositories, track versions, collaborate with others, and securely store code with encryption. The system is implemented in C and provides a command-line interface for all operations.

---

## **Objective**

The main objectives of this project are:

- To provide basic version control functionalities (push, pull, revert, etc.).
- To enable collaboration among multiple users with configurable access control.
- To offer a simple, educational alternative to mainstream VCS tools like Git.
- To demonstrate OS-level concepts such as file management, threading, and user authentication.

---

## **Features**

- **User Authentication**: Register, login, and logout with secure credential management.
- **Repository Management**: Create, browse, and publish repositories.
- **Versioning**: Push new versions, pull any previous version, and revert to older versions.
- **Collaboration**: Add or remove collaborators with fine-grained access control.
- **Stats & Diff**: Compare two versions of a repository and view detailed file differences.
- **Encryption**: Secure files using RC4 encryption/decryption.
- **Threaded Operations**: Uses multithreading for efficient repository browsing and file comparison.
- **Configurable Access**: Public/private repositories and collaborator management via config files.
- **Command-Line Interface**: Interactive menu-driven CLI for all operations.

---

## **How to Use**

1. **Build the Project**

   ```sh
   gcc -o main main.c -lpthread
   ```
2. **Run the Program**

   ```sh
   ./main
   ```
3. **Main Menu Options**

   - Browse public repositories
   - Register as a new user
   - Login/Logout
   - Publish your own repository
   - Push/pull files (with versioning)
   - Add/remove collaborators
4. **Versioning**

   - Each push creates a new `Version_N` folder in the repository.
   - Commit messages and history are tracked in `History.txt`.
   - Pull any version to any folder, or revert to a previous state.
5. **Collaboration**

   - Only authors and collaborators (as listed in `Config.txt`) can push/pull private repositories.
   - Public repositories can be browsed and pulled by anyone.
6. **Encryption**

   - Use the RC4 encryption utility to encrypt/decrypt files as needed.

---

## **Project Structure**

```
ctrl-z/
│
├── main.c            # Main CLI and logic
├── auth.c            # User authentication and registration
├── config.c          # Repository config management
├── encryption.c      # RC4 encryption/decryption
├── stats.c           # Version comparison and diff stats
├── utils.c           # Utility functions (file copy, etc.)
├── version.c         # Versioning logic (push, pull, revert)
│
├── Ascii.txt         # ASCII art for UI
├── command.txt       # Build command
├── readThis.txt      # Team roles and notes
├── README.md         # Project documentation
├── .gitignore        # Ignore compiled binaries
└── .vscode/          # VSCode settings
```

---

## **Technologies Used**

- **Language**: C (C17 standard)
- **Libraries**: POSIX (pthread, dirent, stdio, stdlib, string, sys/stat, unistd)
- **Build Tool**: GCC
- **Editor**: Visual Studio Code

---

## **Contributors**

- **[Abdul Rafay Mughal](https://github.com/abdul-rafay-mughal)**
- **[Abdullah Azhar Khan](https://github.com/abdullahazharkhan)**
- **[Muhammad Awais Shaikh](https://github.com/codexbegin14)**
- **[Syed Muhammad Shubair Hyder](https://github.com/SyedMuhammadShubairHyder)**

---

## **Future Enhancements**

- Add a graphical user interface (GUI).
- Implement more advanced diff and merge tools.
- Add support for remote repositories and networking.
- Improve encryption options and security.
- Add automated tests and CI/CD integration.

---

**Best of luck for the project!**

_PS: This project is for educational purposes and demonstrates core OS and version control concepts in C._
