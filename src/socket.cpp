#include "socket.hpp"
#include "utils.hpp"
#include <iostream>
#include <stdexcept>
#include <netinet/in.h>
#include <thread>
#include <map>
#include <atomic>

using namespace std;

constexpr chrono::milliseconds TIMEOUT_DURATION(10);

void receiveThread(int socket, ThreadSafeQueue<vector<uint8_t>> &queue, sockaddr_in &serverAddress, socklen_t &serverAddressLen);
void processThread(ThreadSafeQueue<vector<uint8_t>> &queue, int socket, sockaddr_in &serverAddress, socklen_t serverAddressLen, SegmentHandler *segmentHandler, uint32_t &lfr, uint32_t &laf, uint32_t rws, atomic<size_t> &totalBytesRead, atomic<uint32_t> &numOfSegmentReceived);

TCPSocket::TCPSocket(const string &ip, int32_t port)
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

    /*
    I think we should actually bind to a specif port and not INADDR_ANY, but somehow it can't receive message from the broadcast
    However, using the debugger, when bound to the INADDR_ANY, address.sin_addr.s_addr has the same value with when it is bound to the input ip
    
    if (inet_pton(AF_INET, this->ip.c_str(), &address.sin_addr) <= 0)
    {
        perror("Invalid IP address");
    }
    */

   address.sin_addr.s_addr = INADDR_ANY;

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

                segmentHandler = new SegmentHandler(DEFAULT_WINDOW_SIZE, ackSeg.acknowledgementNumber, synAckSeg.acknowledgementNumber);

                return {remoteIp, remotePort};
            }
        }
    }
}

string TCPSocket::connect(const string &broadcastAddr, int32_t port)
{
    uint32_t seqNum = rand->getRandomUInt32();
    Segment synSeg = syn(seqNum);
    uint8_t *synSegBuf = serializeSegment(&synSeg, 0, 0);

    struct sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, broadcastAddr.c_str(), &destAddr.sin_addr) <= 0)
    {
        perror("Invalid IP address");
    }

    // enable broadcasting
    int broadcast_value = 1;
    setsockopt(this->socket, SOL_SOCKET, SO_BROADCAST, &broadcast_value, sizeof(broadcast_value));

    cout << "[+] [Handshake] [S=" << seqNum << "] Sending SYN request to " << broadcastAddr << ":" << port << endl;

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
        char remoteIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(remoteAddr.sin_addr), remoteIp, INET_ADDRSTRLEN);

        cout << "[+] [Handshake] [S=" << synAckSeg.sequenceNumber << "] [A=" << synAckSeg.acknowledgementNumber
             << "] Received SYN-ACK request from " << remoteIp << ":" << port << endl;

        Segment ackSeg = ack(synAckSeg.sequenceNumber + 1);
        uint8_t *ackSegBuf = serializeSegment(&ackSeg, 0, 0);

        cout << "[+] [Handshake] [A=" << ackSeg.acknowledgementNumber << "] Sending ACK request to " << remoteIp << ":" << port << endl;

        sendto(this->socket, ackSegBuf, BASE_SEGMENT_SIZE, 0, (struct sockaddr *)&destAddr, sizeof(destAddr));

        segmentHandler = new SegmentHandler(5, ackSeg.acknowledgementNumber, synAckSeg.sequenceNumber);

        this->status = ESTABLISHED;

        // set sliding window attributes
        this->lfr = synAckSeg.sequenceNumber;
        this->laf = this->lfr + this->rws;
         
        return remoteIp;
    }
}

void TCPSocket::send(const string &ip, int32_t port, void *dataStream, uint32_t dataSize)
{
    if (this->status != ESTABLISHED)
    {
        cout << "[i] Connection has not been established" << endl;
        return;
    }
    sockaddr_in clientAddress;
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &clientAddress.sin_addr) <= 0)
    {
        throw runtime_error("Invalid IP address");
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

    cout << "[i] Sending input to " << ip << ":" << port << endl;

    uint32_t numOfSegmentSent = 1;
    this->terminateACK = false;
    bool cont = true;
    while (cont)
    {
        // advance window mengambil segment2 dari  segmentBuffer dan masukin ke window
        this->window = this->segmentHandler->advanceWindow(windowSize);

        vector<Segment *>::iterator myItr;
        for (myItr = this->window.begin(); myItr != this->window.end(); myItr++)
        {
            Segment *tempSeg = *myItr;
            uint32_t seqNum = tempSeg->sequenceNumber;
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
                    cout << "[i] [Established] [Seg "<< numOfSegmentSent << "] [S=" << seqNum << "] Sent" << endl;

                    // masukkan ke map ketika sudah di send
                    sendTimes[seqNum] = chrono::steady_clock::now();
                    this->lfs = seqNum;

                    numOfSegmentSent++;
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

    this->terminateACK = true;
    receiveACK.join();
}

int32_t TCPSocket::recv(void *buffer, uint32_t length)
{
    cout << "[~] [Established] Waiting for segments to be sent" << endl;

    sockaddr_in serverAddress;
    socklen_t serverAddressLen = sizeof(serverAddress);

    ThreadSafeQueue<vector<uint8_t>> packetQueue;

    atomic<size_t> totalBytesRead(0);
    atomic<uint32_t> numOfSegmentReceived(0);

    thread receiver([this, &packetQueue, &serverAddress, &serverAddressLen]()
    {
        receiveThread(this->socket, packetQueue, serverAddress, serverAddressLen);
    });

    thread processor([this, &packetQueue, &serverAddress, &serverAddressLen, &totalBytesRead, &numOfSegmentReceived]()
    {
        uint32_t lfr = this->lfr;
        uint32_t laf = this->laf;
        uint32_t rws = this->rws;
        processThread(packetQueue, this->socket, serverAddress, serverAddressLen, this->segmentHandler, lfr, laf, rws, totalBytesRead, numOfSegmentReceived);
    });

    receiver.join();
    processor.join();

    return totalBytesRead;
}


void TCPSocket::close(const string &ip, int32_t port)
{
    Segment finSeg = fin(this->segmentHandler->getCurrentSeqNum());
    uint8_t *finSegBuf = serializeSegment(&finSeg, 0, 0);

    sockaddr_in clientAddress;
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &clientAddress.sin_addr) <= 0)
    {
        throw runtime_error("Invalid IP address");
    }
    socklen_t clientAddressLen = sizeof(clientAddress);

    cout << "[i] [Closing] Sending FIN request to " << ip << ":" << port << endl;

    sendto(this->socket, finSegBuf, BASE_SEGMENT_SIZE, 0,
            (struct sockaddr *)&clientAddress, clientAddressLen);

    uint8_t recvBuf[BASE_SEGMENT_SIZE];
    memset(recvBuf, 0, BASE_SEGMENT_SIZE);

    ssize_t recvBufLen = recvfrom(this->socket, recvBuf, BASE_SEGMENT_SIZE, 0,
                                    (struct sockaddr *)&clientAddress, &clientAddressLen);

    cout << "[+] [Closing] Received FIN-ACK request from " << ip << ":" << port << endl;

    Segment finAckSeg;
    deserializeToSegment(&finAckSeg, recvBuf, BASE_SEGMENT_SIZE);

    if (flagsToByte(finAckSeg) && finAckSeg.acknowledgementNumber == finSeg.sequenceNumber + 1)
    {
        Segment ackSeg = ack(finAckSeg.sequenceNumber + 1);
        uint8_t * ackSegBuf = serializeSegment(&ackSeg, 0, 0);

        cout << "[i] [Closing] Sending ACK request to " << ip << ":" << port << endl;
    }
}

////server zone
void TCPSocket::listenACK(const string &ip, int32_t port)
{
    while (!this->terminateACK)
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
        if (ip == string(remoteIp) && remotePort == port) // only continue if get ack from the n that we made a handshake with
        {
            if (ackSeg.flags.ack)
            {
                if (ackSeg.acknowledgementNumber > this->lar && ackSeg.acknowledgementNumber <= this->lfs) // check if the ack number is within the window
                {
                    this->lar = ackSeg.acknowledgementNumber;
                }
            }
        }
        serverLock.unlock();
    }
}

// Client zone
void receiveThread(int socket, ThreadSafeQueue<vector<uint8_t>> &queue, sockaddr_in &serverAddress, socklen_t &serverAddressLen)
{
    while (true)
    {
        uint8_t recvBuffer[MAX_SEGMENT_SIZE];
        memset(recvBuffer, 0, sizeof(recvBuffer));

        ssize_t bytesRead = recvfrom(socket, recvBuffer, sizeof(recvBuffer), 0,
                                     (struct sockaddr *)&serverAddress, &serverAddressLen);
        
        if (bytesRead < 0)
        {
            cerr << "Failed to receive data" << endl;
            continue;
        }

        // Store received packet in the queue
        vector<uint8_t> packet(recvBuffer, recvBuffer + bytesRead);
        queue.push(packet);
    }
}

void processThread(ThreadSafeQueue<vector<uint8_t>> &queue, int socket, sockaddr_in &serverAddress, socklen_t serverAddressLen, SegmentHandler *segmentHandler, uint32_t &lfr, uint32_t &laf, uint32_t rws, atomic<size_t> &totalBytesRead, atomic<uint32_t> &numOfSegmentReceived)
{
    while (true)
    {
        vector<uint8_t> packet = queue.pop();

        Segment seg;
        deserializeToSegment(&seg, packet.data(), packet.size());

        if (seg.sequenceNumber <= laf)
        {
            if (seg.sequenceNumber == lfr + 1)
            {
                segmentHandler->appendSegmentBuffer(&seg);
                lfr = seg.sequenceNumber + packet.size() - BASE_SEGMENT_SIZE;

                totalBytesRead.fetch_add(packet.size(), memory_order_relaxed);
                numOfSegmentReceived.fetch_add(1, memory_order_relaxed);
            }

            Segment ackSeg = ack(lfr + 1);
            ssize_t bytesSent = sendto(socket, serializeSegment(&ackSeg, 0, 0), BASE_SEGMENT_SIZE, 0,
                                       (struct sockaddr *)&serverAddress, serverAddressLen);
            if (bytesSent < 0)
            {
                cerr << "Failed to send acknowledgment" << endl;
                continue;
            }

            cout << "[i] [Established] [Seg " << numOfSegmentReceived
                 << "] [A=" << ackSeg.acknowledgementNumber << "] ACK Sent" << endl;
        }
        else
        {
            cout << "[!] [Established] [S=" << seg.sequenceNumber << "] Segment out of range: LAF=" << laf << endl;

            Segment ackSeg = ack(laf);
            ssize_t bytesSent = sendto(socket, serializeSegment(&ackSeg, 0, 0), BASE_SEGMENT_SIZE, 0,
                                       (struct sockaddr *)&serverAddress, serverAddressLen);
            if (bytesSent < 0)
            {
                cerr << "Failed to send acknowledgment" << endl;
                continue;
            }

            cout << "[i] [Established] [Seg " << numOfSegmentReceived
                 << "] [A=" << ackSeg.acknowledgementNumber << "] ACK Sent" << endl;
        }

        laf = lfr + rws;
    }
}