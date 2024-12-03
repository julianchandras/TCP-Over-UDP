#include "socket.hpp"
#include <iostream>
#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "segment_handler.hpp"
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
    if (inet_pton(AF_INET, this->ip.c_str(), &address.sin_addr) <= 0) {
        perror("Invalid IP address");
    }

    bind(this->socket, (struct sockaddr*)&address, sizeof(address));

    // segmentHandler = new SegmentHandler();
    rand = new CSPRNG();
}

TCPSocket::~TCPSocket() {
    // delete segmentHandler;
}

TCPStatusEnum TCPSocket::getStatus() {
    return this->status;
}

void TCPSocket::listen() {
    this->status = LISTEN;

    cout << "[i] Listening to the broadcast port for clients" << endl;

    struct sockaddr_in remoteAddr;
    socklen_t remoteAddrLen = sizeof(remoteAddr);

    uint8_t recvBuf[BASE_SEGMENT_SIZE];
    ssize_t recvBufLen = recvfrom(this->socket, recvBuf, BASE_SEGMENT_SIZE, 0, 
                                  (struct sockaddr*)&remoteAddr, &remoteAddrLen);

    char remoteIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(remoteAddr.sin_addr), remoteIp, INET_ADDRSTRLEN);

    uint16_t remotePort = ntohs(remoteAddr.sin_port);

    Segment synSeg;
    deserializeToSegment(&synSeg, recvBuf, recvBufLen);

    if (flagsToByte(synSeg) == SYN_FLAG) {
        this->status = SYN_RECEIVED;
        uint32_t remoteSeqNum = synSeg.sequenceNumber;

        cout << "[+] [Handshake] [S=" << remoteSeqNum << "] Received SYN request from " << remoteIp << ":" << remotePort << endl;

        uint32_t mySeqNum = rand->getRandomUInt32();
        Segment synAckSeg = synAck(mySeqNum, remoteSeqNum+1);

        uint8_t *synAckSegBuf = serializeSegment(&synAckSeg, 0, 0);

        cout << "[+] [Handshake] [S=" << synAckSeg.sequenceNumber << "] [A=" << synAckSeg.acknowledgementNumber
             << "] Sending SYN-ACK request to " << remoteIp << ":" << remotePort << endl;

        sendto(this->socket, synAckSegBuf, BASE_SEGMENT_SIZE, 0, (struct sockaddr*)&remoteAddr, remoteAddrLen);

        struct sockaddr_in tempRemoteAddr;
        socklen_t tempRemoteAddrLen = sizeof(tempRemoteAddr);

        recvBufLen = recvfrom(this->socket, recvBuf, BASE_SEGMENT_SIZE, 0,
                              (struct sockaddr*)&tempRemoteAddr, &tempRemoteAddrLen);

        char tempRemoteIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(tempRemoteAddr.sin_addr), tempRemoteIp, INET_ADDRSTRLEN);

        uint16_t tempRemotePort = ntohs(tempRemoteAddr.sin_port);

        if (strcmp(remoteIp, tempRemoteIp) == 0 && remotePort == tempRemotePort) {
            Segment ackSeg;
            deserializeToSegment(&ackSeg, recvBuf, recvBufLen);

            if (flagsToByte(ackSeg) == ACK_FLAG) {
                cout << "[+] [Handshake] [A=" << ackSeg.acknowledgementNumber << "] Received ACK request from " << remoteIp << ":" << remotePort << endl;
                
                this->status = ESTABLISHED;
                this->remoteIp = ip;
                this->remotePort =  port;
            }
        }
    }

}

void TCPSocket::connect(string ip, int32_t port) {
    uint32_t seqNum = rand->getRandomUInt32();
    Segment synSeg = syn(seqNum);
    uint8_t *synSegBuf = serializeSegment(&synSeg, 0, 0);
    
    // enable broadcasting
    int broadcast_value = 1;
    setsockopt(this->socket, SOL_SOCKET, SO_BROADCAST, &broadcast_value, sizeof(broadcast_value));

    struct sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &destAddr.sin_addr) <= 0) {
        perror("Invalid IP address");
    }

    cout << "[+] [Handshake] [S=" << seqNum << "] Sending SYN request to " << ip << ":" << port << endl;

    sendto(this->socket, synSegBuf, BASE_SEGMENT_SIZE, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));

    // SYN SENT STATE
    this->status = SYN_SENT;

    struct sockaddr_in remoteAddr;
    socklen_t remoteAddrLen = sizeof(remoteAddr);

    uint8_t recvBuf[BASE_SEGMENT_SIZE];
    ssize_t recvBufLen = recvfrom(this->socket, recvBuf, BASE_SEGMENT_SIZE, 0, 
                                  (struct sockaddr*)&remoteAddr, &remoteAddrLen);

    Segment synAckSeg;
    deserializeToSegment(&synAckSeg, recvBuf, recvBufLen);

    if (flagsToByte(synAckSeg) == SYN_ACK_FLAG && synAckSeg.acknowledgementNumber == ++seqNum) {
        cout << "[+] [Handshake] [S=" << synAckSeg.sequenceNumber << "] [A=" << synAckSeg.acknowledgementNumber
             << "] Received SYN-ACK request from " << ip << ":" << port << endl;

        Segment ackSeg = ack(seqNum, synAckSeg.sequenceNumber+1);
        uint8_t *ackSegBuf = serializeSegment(&ackSeg, 0, 0);

        cout << "[+] [Handshake] [A=" << ackSeg.acknowledgementNumber << "] Sending ACK request to " << ip << ":" << port << endl;

        sendto(this->socket, ackSegBuf, BASE_SEGMENT_SIZE, 0, (struct sockaddr*)&destAddr, sizeof(destAddr));

        this->status = ESTABLISHED;
        this->remoteIp = ip;
        this->remotePort =  port;
    }

}

void TCPSocket::send(string ip, int32_t port, void *dataStream, uint32_t dataSize) {
    if (this->status != ESTABLISHED) {
        cout << "[i] Connection has not been established" << endl;
        return;
    }
    sockaddr_in clientAddress;
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, this->ip.c_str(), &clientAddress.sin_addr) <= 0) {
        perror("Invalid IP address");
    }

    sendto(this->socket, dataStream, dataSize, 0, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
}

int32_t TCPSocket::recv(void *buffer, uint32_t length) {
    sockaddr_in serverAddress;
    socklen_t serverAddressLen = sizeof(serverAddress);

    ssize_t bytesRead = recvfrom(this->socket, buffer, length, 0, 
                                 (struct sockaddr*)&serverAddress, &serverAddressLen);
    if (bytesRead < 0) {
        throw runtime_error("Failed to receive data");
    }

    return bytesRead;
}

void TCPSocket::close() {

}