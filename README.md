# TCP-like Implementation over UDP

A reliable data transfer implementation that brings TCP-like features to UDP sockets, developed as a major project for Computer Network (IF2230) course.

## Project Group: PT-TarunaCitraPratama

### Team Members
- Venantius Sean Ardi Nugroho (13522078)
- Julian Chandra Sutadi (13522080)
- Dimas Bagoes Hendrianto (13522112)
- M. Rifki Virziadeili Harisman (13522120)

## Overview

This project implements TCP's reliable delivery features using UDP socket library. The implementation includes essential TCP mechanisms while maintaining the lightweight nature of UDP.

### Key Features
- Three-way handshake connection establishment
- Sliding window implementation with go-back-N mechanism
- TCP state management following [RFC 9293](https://datatracker.ietf.org/doc/html/rfc9293)
- Proper connection termination

## Technical Details

The enhanced UDP socket with TCP features is implemented in the `TCPSocket` class (`socket.cpp`). While TCP traditionally operates in a peer-to-peer manner, this implementation assumes the `TCPSocket` will be used once by either a `client` or `server` class, both inheriting from the `node` class.

For a visual understanding of the go-back-N mechanism, refer to this [animation](https://www.tkn.tu-berlin.de/teaching/rn/animations/gbn_sr/).

## Prerequisites

- Linux operating system or Windows Subsystem for Linux (WSL)
- GCC compiler
- Make utility

## Installation & Running

1. Clone the repository:
```bash
git clone git@github.com:julianchandras/TCP-Over-UDP.git
cd TCP-Over-UDP
```

2. Build the project:
```bash
make
```

3. Run the application:
```bash
./bin/node [HOST] [PORT]
```

### Testing the Connection

To test the implementation:

1. Open two terminal windows
2. In the first terminal (server):
```bash
./bin/node [HOST] [PORT]
```
3. In the second terminal (client):
```bash
./bin/node [HOST] [PORT]
```

**Important Notes:** 
- The broadcast address selection from the list of available addresses is currently hard-coded at line 30 of `client.cpp`. Modify accordingly and recompile the project if needed.
- Make sure that the `host` and `port` combination is unique.
- Test files can be found in the `filedemo` directory.
- The client will receive data from the server, demonstrating the reliable data transfer implementation

## Project Structure

```
.
├── README.md
├── bin/                      # executable file
├── filedemo/                 # files to be sent during demonstration
├── include/                  # header files
├── makefile
├── obj/                      # object files
└── src
    ├── CSPRING.cpp           # random number generator
    ├── client.cpp            # client-side implementation
    ├── node.cpp              # base class for client/server nodes
    ├── segment.cpp           # TCP segment structure implementation
    ├── segment_handler.cpp   # handles segment processing and management
    ├── server.cpp            # server-side implementation
    ├── socket.cpp            # TCPSocket implementation with TCP-like features
    └── utils.cpp             # utility functions (segment serializer and deserializer)
```