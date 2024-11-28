#include "socket.hpp"
#include <iostream>
#include <stdexcept>
#include <netinet/in.h>
#include "utils.hpp"

using namespace std;

TCPSocket::TCPSocket(string ip, int32_t port) {
    this->ip = ip;
    this->port = port;

    this->socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (this->socket == -1) {
        throw new runtime_error("Failed to create socket");
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(this->port);
    address.sin_addr.s_addr = stoi(this->ip);

    bind(this->socket, (struct sockaddr*)&address, sizeof(address));

    segmentHandler = new SegmentHandler();
}

TCPSocket::~TCPSocket() {
    delete segmentHandler;
}

TCPStatusEnum TCPSocket::getStatus() {
    return this->status;
}

void TCPSocket::listen() {
    this->status = TCPStatusEnum::LISTEN;

    // this should be for the client (sending req to server), not listening server
    // int broadcast_value = 1;
    // setsockopt(this->socket, SOL_SOCKET, SO_BROADCAST, &broadcast_value, sizeof(broadcast_value));

    cout << "[i] Listening to the broadcast port for clients." << endl;
}

void TCPSocket::connect(string ip, int32_t port) {
    Segment synSeg = syn(rand->getRandomUInt32());
    uint8_t *synSegBuf = serializeSegment(&synSeg, 0, 0);

    sendto(this->socket, synSegBuf, sizeof(synSegBuf))
}

void TCPSocket::send(string ip, int32_t port, void *dataStream, uint32_t dataSize) {
    sockaddr_in clientAddress;
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_port = htons(port);
    clientAddress.sin_addr.s_addr = stoi(ip);

    sendto(this->socket, dataStream, dataSize, 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
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