
# wsted

**wsted** is a program for transferring files between users within separate rooms. The project consists of a client and a server written in C++ using the Qt 6 framework.

Please note that this is done for educational practice and has some flaws by design.

![GNU/Linux](https://img.shields.io/badge/Linux-FCC624.svg?style=for-the-badge&logo=Linux&logoColor=black)
## Screenshots

![App Screenshot](https://images2.imgbox.com/a9/52/BfSBIXGP_o.png)


## Features

The server does not specifically use any databases. User and room data are stored in memory as long as the server is running. The idea behind this implementation is that the server owner has as little information about the clients as possible.

The server deals with:
- generating new rooms if users request it
- storing and sending files
- receiving and forwarding messages

The server knows nothing about the current state of the client except that it is connected to a socket.

The client deals with:
- sending and receiving messages
- sending and receiving files

Interaction between the client and the server is limited by commands like "/dosomething".

## Installation

Required packages:
- Qt 6 Base >= 6.4 (Arch - `qt6-base`, Debian - `qt6-base-dev`)
- Qt 6 Compat >= 6.4 (Arch - `qt6-5compat`, Debian - `qt6-5compat-dev`)
- CMake

```bash
git clone https://github.com/meequrox/wsted.git

cd wsted

mkdir build && cd build

cmake ..

make
```
    
## Usage

```bash
# Run server on default port
./wsted-server

# Run server on custom port
./wsted-server 7999

# Run client
./wsted-client
```
