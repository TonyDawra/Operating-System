# Multi-Client Chat Application in C

A robust, multi-threaded chat application implemented in C, allowing multiple clients to communicate through a central server using named pipes (FIFOs) and TCP sockets. The system incorporates synchronization mechanisms to prevent race conditions, deadlock, and starvation, and includes security measures against brute‑force attacks. citeturn4file1

---

## Table of Contents

1. [Features](#features)
2. [Prerequisites](#prerequisites)
3. [Project Structure](#project-structure)
4. [Build Instructions](#build-instructions)
5. [Usage](#usage)

   * [Server](#server)
   * [Client](#client)
6. [Configuration](#configuration)
7. [Synchronization & Security](#synchronization--security)
8. [Helper Scripts](#helper-scripts)
9. [Contributing](#contributing)
10. [License](#license)

---

## Features

* **Multi-threaded server** handling each client in a separate thread. citeturn4file6
* **Named pipes (FIFOs)** and TCP sockets for reliable communication. citeturn4file1
* **Synchronization** using mutex locks to manage shared client list and message queues. citeturn4file6
* **Robust I/O** with Rio library functions (`rio_readn`, `rio_writen`, `rio_readlineb`). citeturn4file4turn4file5
* **Commands**:

  * `msg "text"` send to all clients
  * `msg "text" user` send to specific client
  * `online` list active users
  * `quit` disconnect gracefully citeturn4file6

---

## Prerequisites

* GCC or Clang with C11 support
* POSIX threads (`-pthread`)
* Standard C libraries: `<pthread.h>`, `<netdb.h>`, `<sys/socket.h>`, `<unistd.h>`, `<errno.h>`

---

## Project Structure

```
├── server.c       # Multi-threaded server implementation
├── client.c       # Client application with user input and server response reader
├── helper.c/.h    # Robust Rio I/O functions (rio_readn, rio_writen, rio_readlineb)
├── functions.h    # Connection helper prototypes
├── getIp.py       # Python script to retrieve local machine IP address
├── Makefile       # Build rules for server and client
└── Final Project.pdf  # Project report and design rationale citeturn4file1
```

---

## Build Instructions

Use the provided Makefile to compile both server and client:

```bash
# In project root
git clone <repo_url> && cd <project_folder>
make all
```

This produces:

* `server` executable
* `client` executable

To clean build artifacts:

```bash
make clean
```

---

## Usage

### Build

Compile the server and client using GCC:

```bash
gcc -o client client.c helper.c
gcc -o server server.c helper.c
```

### Run Helper Script

Fetch the host IP address:

```bash
python3 getIp.py
```

### Server

1. Start the server (default port 80):

   ```bash
   ./server [port]
   ```

### Client

1. Run the client with server address, port, and your username:

   ```bash
   ./client -a <ip> -p <port> -u <username>
   ```

---

## Configuration

* **Port**: Default server port is `80`, can be overridden via `argv[1]`.
* **MAXLINE**: Maximum message length in `client.c` (default 1024 bytes). citeturn4file0

---

## Synchronization & Security

* **Mutex** protects global client list to prevent data races. citeturn4file6
* **Graceful disconnect** ensures resources are freed and other clients are notified. citeturn4file6
* **Error handling** on socket operations and I/O (retry on `EINTR`). citeturn4file4
* **Security considerations** in report: brute-force attack mitigation and unauthorized access prevention. citeturn4file1

---

## Helper Scripts

* `getIp.py`: Python script to print the host’s IP address for client convenience. Run:

  ````bash
  python3 getIp.py
  ``` citeturn4file3
  ````

---

## Contributing

1. Fork the repository.
2. Create a branch: `git checkout -b feature/YourFeature`
3. Commit your changes with clear messages.
4. Open a Pull Request for review.

---

## License

This project is released under the MIT License. See `LICENSE` for details.
