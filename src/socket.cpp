#include "socket.hpp"
#include <iostream>
#include <stdexcept>
#include <netinet/in.h>

using namespace std;

TCPSocket::TCPSocket(string ip, int32_t port) {
    this->ip = ip;
    this->port = port;

    this->socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (this->socket == -1) {
        throw new runtime_error("Failed to create socket");
    }
}

TCPStatusEnum TCPSocket::getStatus() {
    return this->status;
}

void TCPSocket::listen() {
    this->status = TCPStatusEnum::LISTEN;

    sockaddr_in clientAddress;
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_port = htons(this->port);
    clientAddress.sin_addr.s_addr = stoi(this->ip);

    bind(this->socket, (struct sockaddr*)&clientAddress, sizeof(clientAddress));

    // cout << "[i] Listening to the broadcast port for clients." << endl;
    cout << "[i] Listening to the port for messages." << endl;
}

void TCPSocket::send(string ip, int32_t port, void *dataStream, uint32_t dataSize) {
    int32_t clientSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (this->socket == -1) {
        throw new runtime_error("Failed to create socket");
    }

    sockaddr_in clientAddress;
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_port = htons(port);
    clientAddress.sin_addr.s_addr = stoi(ip);

    sendto(clientSocket, dataStream, dataSize, 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
}

int32_t TCPSocket::recv(void *buffer, uint32_t length) {
    sockaddr_in serverAddress;
    socklen_t serverAddressLen = sizeof(serverAddress);

    int32_t bytesRead = recvfrom(this->socket, buffer, length, 0, 
                                 (struct sockaddr*)&serverAddress, &serverAddressLen);
    if (bytesRead < 0) {
        throw runtime_error("Failed to receive data");
    }

    return bytesRead;
}

void TCPSocket::close() {

}