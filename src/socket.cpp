#include "socket.hpp"
#include <iostream>
#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>
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

    this->rand = new CSPRNG();
}

TCPSocket::~TCPSocket()
{
    delete segmentHandler;
}

TCPStatusEnum TCPSocket::getStatus() {
    return this->status;
}

void TCPSocket::listen()
{
    this->status = LISTEN;

    cout << "[i] Listening to the broadcast port for clients" << endl;

    struct sockaddr_in remoteAddr;
    socklen_t remoteAddrLen = sizeof(remoteAddr);

    uint8_t recvBuf[BASE_SEGMENT_SIZE];
    ssize_t recvBufLen = recvfrom(this->socket, recvBuf, BASE_SEGMENT_SIZE, 0,
                                  (struct sockaddr *)&remoteAddr, &remoteAddrLen);

    char remoteIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(remoteAddr.sin_addr), remoteIp, INET_ADDRSTRLEN);

    uint16_t remotePort = ntohs(remoteAddr.sin_port);

    Segment synSeg;
    deserializeToSegment(&synSeg, recvBuf, recvBufLen);

    if (flagsToByte(synSeg) == SYN_FLAG)
    {
        this->status = SYN_RECEIVED;
        uint32_t remoteSeqNum = synSeg.sequenceNumber;

        cout << "[+] [Handshake] [S=" << remoteSeqNum << "] Received SYN request from " << remoteIp << ":" << remotePort << endl;

        uint32_t mySeqNum = rand->getRandomUInt32();
        Segment synAckSeg = synAck(mySeqNum, remoteSeqNum + 1);

        uint8_t *synAckSegBuf = serializeSegment(&synAckSeg, 0, 0);

        cout << "[+] [Handshake] [S=" << synAckSeg.sequenceNumber << "] [A=" << synAckSeg.acknowledgementNumber
             << "] Sending SYN-ACK request to " << remoteIp << ":" << remotePort << endl;

        sendto(this->socket, synAckSegBuf, BASE_SEGMENT_SIZE, 0, (struct sockaddr *)&remoteAddr, remoteAddrLen);

        struct sockaddr_in tempRemoteAddr;
        socklen_t tempRemoteAddrLen = sizeof(tempRemoteAddr);

        recvBufLen = recvfrom(this->socket, recvBuf, BASE_SEGMENT_SIZE, 0,
                              (struct sockaddr *)&tempRemoteAddr, &tempRemoteAddrLen);

        char tempRemoteIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(tempRemoteAddr.sin_addr), tempRemoteIp, INET_ADDRSTRLEN);

        uint16_t tempRemotePort = ntohs(tempRemoteAddr.sin_port);

        if (strcmp(remoteIp, tempRemoteIp) == 0 && remotePort == tempRemotePort)
        {
            Segment ackSeg;
            deserializeToSegment(&ackSeg, recvBuf, recvBufLen);

            if (flagsToByte(ackSeg) == ACK_FLAG)
            {
                cout << "[+] [Handshake] [A=" << ackSeg.acknowledgementNumber << "] Received ACK request from " << remoteIp << ":" << remotePort << endl;

                this->status = ESTABLISHED;
                this->remoteIp = ip;
                this->remotePort = port;

                // Is this correct? Our seq is the receiver ack and our ack is the receiver seq
                segmentHandler = new SegmentHandler(5, ackSeg.acknowledgementNumber, ackSeg.sequenceNumber);
            }
        }
    }
}

void TCPSocket::connect(string ip, int32_t port)
{
    uint32_t seqNum = rand->getRandomUInt32();
    Segment synSeg = syn(seqNum);
    uint8_t *synSegBuf = serializeSegment(&synSeg, 0, 0);

    // enable broadcasting
    int broadcast_value = 1;
    setsockopt(this->socket, SOL_SOCKET, SO_BROADCAST, &broadcast_value, sizeof(broadcast_value));

    cout << "[i] Listening to the broadcast port for clients." << endl;
}

void TCPSocket::connect(string ip, int32_t port) {
    Segment synSeg = syn(rand->getRandomUInt32());

    // sendto(this->socket, )
}

void TCPSocket::send(string ip, int32_t port, void *dataStream, uint32_t dataSize) {
    sockaddr_in clientAddress;
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, this->ip.c_str(), &clientAddress.sin_addr) <= 0)
    {
        perror("Invalid IP address");
    }

    this->window.clear();

    // Experimenting with segment handler
    this->segmentHandler->setDataStream((uint8_t *)dataStream, dataSize);
    this->segmentHandler->generateSegments();

    uint8_t initWindowSize = this->segmentHandler->getWindowSize();
    uint8_t windowSize = initWindowSize;

    bool cont = true;
    while (cont)
    {
        this->window = this->segmentHandler->advanceWindow(windowSize);

        vector<Segment*>::iterator myItr;
        for (myItr = this->window.begin(); myItr != this->window.end(); myItr++)
        {
            uint8_t *buffer = serializeSegment(*myItr, 0, MAX_PAYLOAD_SIZE);
            sendto(this->socket, buffer, MAX_SEGMENT_SIZE, 0, (struct sockaddr *)&clientAddress, sizeof(clientAddress));
        }

        if (this->window.empty())
        {
            cont = false;
        }
    }
    // vector<Segment>::iterator myItr;

    // for (myItr = this->segmentBuffer.begin(); myItr != this->segmentBuffer.end(); myItr++)
    // {
    //     cout << myItr->payload << " " << endl
    //             << i << endl;
    //     uint8_t *buffer = serializeSegment(&(*myItr), 0, 1460);
    //     socket->send("127.0.0.1", 5679, buffer, 1460);
    //     i++;
    // }

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

void TCPSocket::close()
{
}

uint32_t TCPSocket::getRandomSeqNum()
{
    return this->rand->getRandomUInt32();
}