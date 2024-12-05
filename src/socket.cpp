#include "socket.hpp"
#include <iostream>
#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <chrono>
#include <map>
#include "utils.hpp"

using namespace std;

constexpr std::chrono::milliseconds TIMEOUT_DURATION(10);

TCPSocket::TCPSocket(string ip, int32_t port)
{
    this->ip = ip;
    this->port = port;

    this->socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (this->socket == -1)
    {
        throw new runtime_error("Failed to create socket");
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(this->port);
    if (inet_pton(AF_INET, this->ip.c_str(), &address.sin_addr) <= 0)
    {
        perror("Invalid IP address");
    }

    bind(this->socket, (struct sockaddr *)&address, sizeof(address));

    this->rand = new CSPRNG();
}

TCPSocket::~TCPSocket()
{
    delete segmentHandler;
}

TCPStatusEnum TCPSocket::getStatus()
{
    return this->status;
}

pair<string, int32_t> TCPSocket::listen()
{
    this->status = LISTEN;

    cout << "[i] Listening to the broadcast port for clients" << endl;

    struct sockaddr_in remoteAddr;
    socklen_t remoteAddrLen = sizeof(remoteAddr);

    uint8_t recvBuf[BASE_SEGMENT_SIZE];
    ssize_t recvBufLen = recvfrom(this->socket, recvBuf, BASE_SEGMENT_SIZE, 0,
                                  (struct sockaddr *)&remoteAddr, &remoteAddrLen);

    char remoteIp[INET_ADDRSTRLEN]; // extract ip
    inet_ntop(AF_INET, &(remoteAddr.sin_addr), remoteIp, INET_ADDRSTRLEN);

    uint16_t remotePort = ntohs(remoteAddr.sin_port); // extract port

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

                segmentHandler = new SegmentHandler(5, ackSeg.acknowledgementNumber, ackSeg.sequenceNumber);

                return {remoteIp, remotePort};
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

    struct sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &destAddr.sin_addr) <= 0)
    {
        perror("Invalid IP address");
    }

    cout << "[+] [Handshake] [S=" << seqNum << "] Sending SYN request to " << ip << ":" << port << endl;

    sendto(this->socket, synSegBuf, BASE_SEGMENT_SIZE, 0, (struct sockaddr *)&destAddr, sizeof(destAddr));

    // SYN SENT STATE
    this->status = SYN_SENT;

    struct sockaddr_in remoteAddr;
    socklen_t remoteAddrLen = sizeof(remoteAddr);

    uint8_t recvBuf[BASE_SEGMENT_SIZE];
    ssize_t recvBufLen = recvfrom(this->socket, recvBuf, BASE_SEGMENT_SIZE, 0,
                                  (struct sockaddr *)&remoteAddr, &remoteAddrLen);

    Segment synAckSeg;
    deserializeToSegment(&synAckSeg, recvBuf, recvBufLen);

    if (flagsToByte(synAckSeg) == SYN_ACK_FLAG && synAckSeg.acknowledgementNumber == ++seqNum)
    {
        cout << "[+] [Handshake] [S=" << synAckSeg.sequenceNumber << "] [A=" << synAckSeg.acknowledgementNumber
             << "] Received SYN-ACK request from " << ip << ":" << port << endl;

        Segment ackSeg = ack(seqNum, synAckSeg.sequenceNumber + 1);
        uint8_t *ackSegBuf = serializeSegment(&ackSeg, 0, 0);

        cout << "[+] [Handshake] [A=" << ackSeg.acknowledgementNumber << "] Sending ACK request to " << ip << ":" << port << endl;

        sendto(this->socket, ackSegBuf, BASE_SEGMENT_SIZE, 0, (struct sockaddr *)&destAddr, sizeof(destAddr));

        segmentHandler = new SegmentHandler(5, ackSeg.acknowledgementNumber, synAckSeg.sequenceNumber);

        this->status = ESTABLISHED;
    }
}

void TCPSocket::send(string ip, int32_t port, void *dataStream, uint32_t dataSize)
{
    if (this->status != ESTABLISHED)
    {
        cout << "[i] Connection has not been established" << endl;
        return;
    }
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
    // tcp socket yang dimiliki node, memiliki segment handler
    // segment handler ada segment buffer
    // dengan generateSegments, segment buffer diisi dengan segment yang di set dari setDataStream
    this->segmentHandler->generateSegments();

    uint8_t initWindowSize = this->segmentHandler->getWindowSize();
    uint8_t windowSize = initWindowSize;
    
    thread receiveACK([this, ip, port]() { this->listenACK(ip, port); });
    map<int, chrono::time_point<chrono::steady_clock>> sendTimes;

    // kita bisa menggunakan dataSize yang dikurang tiap kali paket terikirim.
    // Jika ternyata data size sudah < max-payload_size artinya kita serialize based on size itu aja

    bool cont = true;
    while (cont)
    {
        // advance window mengambil segment2 dari  segmentBuffer dan masukin ke window
        this->window = this->segmentHandler->advanceWindow(windowSize);

        vector<Segment *>::iterator myItr;
        for (myItr = this->window.begin(); myItr != this->window.end(); myItr++)
        {
            Segment *tempSeg = *myItr;
            int seqNum = tempSeg->sequenceNumber;
            // kalau belum ada di map atau melebihi timeout
            if (sendTimes.find(seqNum) == sendTimes.end() || chrono::steady_clock::now() - sendTimes[seqNum] > TIMEOUT_DURATION)
            {
                uint8_t *buffer = serializeSegment(*myItr, 0, MAX_PAYLOAD_SIZE);
                ssize_t sentLen = sendto(this->socket, buffer, MAX_SEGMENT_SIZE, 0, (struct sockaddr *)&clientAddress, sizeof(clientAddress));
                if (sentLen < 0)
                {
                    perror("Error sending segment");
                }
                else
                {
                    // masukkan ke map ketika sudah di send
                    sendTimes[seqNum] = std::chrono::steady_clock::now();
                    this->lfs = seqNum;
                }
            }
        }
        // calculate window size
        serverLock.lock();
        windowSize = (this->lfs - this->lar) / MAX_SEGMENT_SIZE;
        serverLock.unlock();
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

    sendto(this->socket, dataStream, dataSize, 0, (struct sockaddr *)&clientAddress, sizeof(clientAddress));
    receiveACK.join();
}

int32_t TCPSocket::recv(void *buffer, uint32_t length)
{
    sockaddr_in serverAddress;
    socklen_t serverAddressLen = sizeof(serverAddress);

    uint8_t recvBuffer[length];
    memset(recvBuffer, 0, length);

    ssize_t bytesRead = recvfrom(this->socket, recvBuffer, length, 0,
                                 (struct sockaddr *)&serverAddress, &serverAddressLen);
    if (bytesRead < 0)
    {
        throw runtime_error("Failed to receive data");
    }
    this->segmentHandler->appendSegmentBuffer(recvBuffer, bytesRead);
    this->segmentHandler->getDatastream((uint8_t *)buffer, length);

    return bytesRead;
}

void TCPSocket::close()
{
}

////server zone
void TCPSocket::listenACK(string ip, int32_t port)
{
    while (true)
    {
        struct sockaddr_in remoteAddr;
        socklen_t remoteAddrLen = sizeof(remoteAddr);
        uint8_t recvBuf[BASE_SEGMENT_SIZE];
        ssize_t recvBufLen = recvfrom(this->socket, recvBuf, BASE_SEGMENT_SIZE, 0,
                                      (struct sockaddr *)&remoteAddr, &remoteAddrLen);
        Segment ackSeg;
        deserializeToSegment(&ackSeg, recvBuf, recvBufLen);
        char remoteIp[INET_ADDRSTRLEN];                   // extract ip
        uint16_t remotePort = ntohs(remoteAddr.sin_port); // extract port
        serverLock.lock();
        if (ip == std::string(remoteIp) && remotePort == port) // only continue if get ack from the n that we made a handshake with
        {
            if (ackSeg.flags.ack)
            {
                if (ackSeg.acknowledgementNumber > this->lar && ackSeg.acknowledgementNumber <= this->lfs) // check if the ack number is within the window
                {
                    this->lar = ackSeg.acknowledgementNumber;
                }
            }
        }
        else
        {
            cout << "hah" << endl;
        }
        serverLock.unlock();
    }
}
