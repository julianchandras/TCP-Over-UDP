#include "socket.hpp"
#include "utils.hpp"
#include <iostream>
#include <stdexcept>
#include <netinet/in.h>
#include <thread>
#include <map>
#include <cmath>

using namespace std;

constexpr chrono::milliseconds TIMEOUT_DURATION(1000);

TCPSocket::TCPSocket(const string &ip, int32_t port)
{
    this->ip = ip;
    this->port = port;

    this->socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (this->socket == -1)
    {
        throw new runtime_error("Failed to create socket");
    }
    timeval timeout;
    timeout.tv_sec = 10; // 10 seconds timeout
    timeout.tv_usec = 0;
    if (setsockopt(this->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        throw runtime_error("Failed to set receive timeout");
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

        int retryCount = 0;
        const int timeout = 3;

        while (retryCount < MAX_RETRIES)
        {
            sendto(this->socket, synAckSegBuf, BASE_SEGMENT_SIZE, 0, (struct sockaddr *)&remoteAddr, remoteAddrLen);

            struct sockaddr_in tempRemoteAddr;
            socklen_t tempRemoteAddrLen = sizeof(tempRemoteAddr);

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(this->socket, &readfds);

            struct timeval tv;
            tv.tv_sec = timeout;
            tv.tv_usec = 0;

            // Wait for incoming data with timeout
            int selectResult = select(this->socket + 1, &readfds, NULL, NULL, &tv);

            if (selectResult > 0)
            {
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


                        this->lar = ackSeg.acknowledgementNumber;
                        this->status = ESTABLISHED;

                        segmentHandler = new SegmentHandler(DEFAULT_WINDOW_SIZE, ackSeg.acknowledgementNumber, synAckSeg.acknowledgementNumber);

                        // turns on non blocking mode after Handshake is formed
                        int flags = fcntl(this->socket, F_GETFL, 0);
                        if (flags == -1)
                        {
                            throw runtime_error("Failed to get socket flags");
                        }
                        if (fcntl(this->socket, F_SETFL, flags | O_NONBLOCK) == -1)
                        {
                            throw runtime_error("Failed to set socket to non-blocking mode");
                        }

                        cout << "[i] Socket set to non-blocking mode\n";

                        return {remoteIp, remotePort};
                    }
                }
            }
            else
            {
                retryCount++;
                cout << "[!] [Handshake] No ACK received. Retrying... (" << retryCount << "/" << MAX_RETRIES << ")\n";
            }
        }
        cout << "[!] [Handshake] Failed after " << MAX_RETRIES << " retries." << endl;
        this->status = LISTEN;
    }

    return {};
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
        return "";
    }

    // enable broadcasting
    int broadcast_value = 1;
    setsockopt(this->socket, SOL_SOCKET, SO_BROADCAST, &broadcast_value, sizeof(broadcast_value));

    cout << "[+] [Handshake] [S=" << seqNum << "] Sending SYN request to " << broadcastAddr << ":" << port << endl;

    int retryCount = 0;
    while (retryCount < MAX_RETRIES)
    {
        ssize_t bytesSent = sendto(this->socket, synSegBuf, BASE_SEGMENT_SIZE, 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (bytesSent < 0)
        {
            cerr << "[!] [Handshake] Failed to send SYN request. Retrying..." << endl;
            retryCount++;
            continue;
        }

        // SYN SENT STATE
        this->status = SYN_SENT;

        struct sockaddr_in remoteAddr;
        socklen_t remoteAddrLen = sizeof(remoteAddr);
        uint8_t recvBuf[BASE_SEGMENT_SIZE];

        ssize_t recvBufLen = recvfrom(this->socket, recvBuf, BASE_SEGMENT_SIZE, 0,
                                      (struct sockaddr *)&remoteAddr, &remoteAddrLen);
        if (recvBufLen < 0)
        {
            cerr << "[!] [Handshake] Failed to receive data. Retrying..." << endl;
            retryCount++;
            continue;
        }
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

            cout << "[+] [Handshake] [A=" << ackSeg.acknowledgementNumber << "] Sending ACK to " << remoteIp << ":" << port << endl;
            ssize_t ackBytesSent = sendto(this->socket, ackSegBuf, BASE_SEGMENT_SIZE, 0, (struct sockaddr *)&destAddr, sizeof(destAddr));

            if (ackBytesSent < 0)
            {
                cerr << "[!] [Handshake] Failed to send ACK request. Retrying..." << endl;
                retryCount++;
                continue;
            }
            segmentHandler = new SegmentHandler(DEFAULT_WINDOW_SIZE, ackSeg.acknowledgementNumber, synAckSeg.sequenceNumber);

            this->status = ESTABLISHED;

            // set sliding window attributes
            this->lfr = synAckSeg.sequenceNumber;
            this->laf = this->lfr + this->rws;

            return remoteIp;
        }
        else
        {
            cerr << "[!] [Handshake] SYN-ACK mismatch or invalid acknowledgement number. Retrying..." << endl;
            retryCount++;
        }
    }
    cerr << "[!] [Handshake] Max retries reached. Could not establish connection." << endl;
    return "";
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
    // masukin data ke datastream
    this->segmentHandler->setDataStream((uint8_t *)dataStream, dataSize);
    // masukin ke segment buffer
    this->segmentHandler->generateSegments();
    // will just get the base window size, which is 7
    uint8_t windowSize = this->segmentHandler->getWindowSize();
    // name will make sense later
    chrono::time_point<chrono::steady_clock> timeoutCountStart = chrono::steady_clock::now();
    cout << "[i] [Established] Sending input to " << ip << ":" << port << endl;
    std::set<uint32_t> sentSegment;
    // advance window mengambil segment2 dari  segmentBuffer dan masukin ke window
    this->segmentHandler->advanceWindow(windowSize, &this->window);
    size_t segmentBufferSize = this->segmentHandler->getSegmentBufferSize();
    size_t ackReceived = 0;
    while (ackReceived < segmentBufferSize)
    {
        // sending part
        // cout << "Remaining data size: " << remainingDataSize << endl;
        //  cout << "First Index of window: " << firstIndexofWindow << endl;
        //  cout << "window size: " << this->window.size() << endl;
        //  cout << firstIndexofWindow << "\n";
        for (size_t i = 0; i < this->window.size(); i++)
        {
            Segment *tempSeg = window[i];
            uint32_t seqNum = tempSeg->sequenceNumber;

            bool retransmit = (chrono::steady_clock::now() - timeoutCountStart > TIMEOUT_DURATION) ? true : false;

            if (sentSegment.find(seqNum) == sentSegment.end() || retransmit)
            {
                uint32_t payloadSize;
                size_t sentSegmentNum = retransmit ? ackReceived + i + 1 : ackReceived + sentSegment.size() + 1;
                if (segmentBufferSize - sentSegmentNum > 1)
                {
                    payloadSize = MAX_PAYLOAD_SIZE;
                }
                else
                {
                    payloadSize = dataSize - ((sentSegmentNum - 1) * MAX_PAYLOAD_SIZE);
                }

                uint8_t *buffer = serializeSegment(tempSeg, 0, payloadSize);
                ssize_t sentLen = sendto(this->socket, buffer, payloadSize + BASE_SEGMENT_SIZE, 0, (struct sockaddr *)&clientAddress, sizeof(clientAddress));

                if (sentLen < 0)
                {
                    perror("Error sending segment");
                }
                else
                {
                    cout << "[i] [Established] [Seg " << sentSegmentNum << "] [S=" << seqNum << "] Sent" << endl;
                    if (retransmit && (i == 0))
                    {
                        timeoutCountStart = chrono::steady_clock::now();
                    }
                    sentSegment.insert(seqNum);
                    this->lfs = seqNum + payloadSize;
                    delete[] buffer;
                }
            }
        }
        // receiving ACK part
        struct sockaddr_in remoteAddr;
        socklen_t remoteAddrLen = sizeof(remoteAddr);
        uint8_t recvBuf[BASE_SEGMENT_SIZE];
        ssize_t recvBufLen = recvfrom(this->socket, recvBuf, BASE_SEGMENT_SIZE, 0,
                                      (struct sockaddr *)&remoteAddr, &remoteAddrLen);
        if (recvBufLen > 0)
        {
            Segment ackSeg;
            deserializeToSegment(&ackSeg, recvBuf, recvBufLen);

            if (ackSeg.flags.ack)
            {
                if (ackSeg.acknowledgementNumber > this->lar && ackSeg.acknowledgementNumber <= this->lfs + 1)
                {
                    cout << "[!] [Established] [A=" << ackSeg.acknowledgementNumber << "] ACK Received" << endl;

                    // Remove segments with seqNum < ackNumber from sentSegment
                    for (auto it = sentSegment.begin(); it != sentSegment.end();)
                    {
                        if (*it < ackSeg.acknowledgementNumber)
                        {
                            it = sentSegment.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }

                    // Move the window forward
                    uint32_t numSegmentsAcked = ceil((float) (ackSeg.acknowledgementNumber - this->lar) / MAX_PAYLOAD_SIZE);
                    this->segmentHandler->advanceWindow(numSegmentsAcked, &this->window);
                    
                    ackReceived += numSegmentsAcked;
                    this->window.erase(window.begin(), window.begin() + numSegmentsAcked);

                    this->lar = ackSeg.acknowledgementNumber;
                    timeoutCountStart = chrono::steady_clock::now();
                }
            }
        }
    }
}

int32_t TCPSocket::recv(std::vector<uint8_t> &dataStream)
{
    cout << "[~] [Established] Waiting for segments to be sent" << endl;

    sockaddr_in serverAddress;
    socklen_t serverAddressLen = sizeof(serverAddress);

    ThreadSafeQueue<vector<uint8_t>> packetQueue;

    atomic<uint32_t> numOfSegmentReceived(0);

    this->isReceiving = true;

    thread receiver([this, &serverAddress, &serverAddressLen]()
                    { this->receiveThread(serverAddress, serverAddressLen); });

    thread processor([this, &serverAddress, &serverAddressLen, &numOfSegmentReceived]()
                     { this->processThread(serverAddress, serverAddressLen, numOfSegmentReceived); });

    receiver.join();
    processor.join();

    this->segmentHandler->getDatastream(dataStream);

    return this->totalBytesRead;
}

void TCPSocket::close(const string &ip, int32_t port)
{
    Segment finSeg = fin(this->lfs);
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

    // Send FIN request
    ssize_t bytesSent = sendto(this->socket, finSegBuf, BASE_SEGMENT_SIZE, 0,
                               (struct sockaddr *)&clientAddress, clientAddressLen);
    if (bytesSent < 0)
    {
        throw runtime_error("Failed to send FIN request");
    }

    uint8_t recvBuf[BASE_SEGMENT_SIZE];
    ssize_t recvBufLen;

    int retryCount = 0;
    const int timeout = 5000;

    this->status = FIN_WAIT_1;
    while (retryCount < MAX_RETRIES && this->status != TIMED_WAIT)
    {
        recvBufLen = recvfrom(this->socket, recvBuf, BASE_SEGMENT_SIZE, 0,
                              (struct sockaddr *)&clientAddress, &clientAddressLen);

        if (recvBufLen < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // No data available, wait and retry
                this_thread::sleep_for(chrono::milliseconds(timeout));
                retryCount++;
                cout << "[i] [Closing] Retrying (" << retryCount << "/" << MAX_RETRIES << ")" << endl;
                sendto(this->socket, finSegBuf, BASE_SEGMENT_SIZE, 0,
                       (struct sockaddr *)&clientAddress, clientAddressLen);
                continue;
            }
            else
            {
                cerr << "[!] [Closing] Failed to receive data, error: " << strerror(errno) << endl;
                return;
            }
        }

        Segment receivedSeg;
        deserializeToSegment(&receivedSeg, recvBuf, BASE_SEGMENT_SIZE);

        // Check if the packet is a valid FIN-ACK and the sequence number matches
        if (flagsToByte(receivedSeg) == FIN_ACK_FLAG && receivedSeg.acknowledgementNumber == finSeg.sequenceNumber + 1)
        {
            cout << "[+] [Closing] Received FIN-ACK request from " << ip << ":" << port << endl;

            // Send ACK response to finalize the closure
            Segment ackSeg = ack(receivedSeg.sequenceNumber + 1);
            uint8_t *ackSegBuf = serializeSegment(&ackSeg, 0, 0);

            cout << "[i] [Closing] Sending ACK to " << ip << ":" << port << endl;
            ssize_t ackSent = sendto(this->socket, ackSegBuf, BASE_SEGMENT_SIZE, 0,
                                     (struct sockaddr *)&clientAddress, clientAddressLen);
            if (ackSent < 0)
            {
                cerr << "[!] [Closing] Failed to send ACK" << endl;
                return;
            }

            this->status = TIMED_WAIT;
        }
        else if (flagsToByte(receivedSeg) == FIN_FLAG)
        {
            Segment ackSeg = ack(receivedSeg.sequenceNumber + 1);
            uint8_t *ackSegBuf = serializeSegment(&ackSeg, 0, 0);

            cout << "[i] [Closing] Sending ACK to " << ip << ":" << port << endl;
            ssize_t ackSent = sendto(this->socket, ackSegBuf, BASE_SEGMENT_SIZE, 0,
                                        (struct sockaddr *)&clientAddress, clientAddressLen);
            if (ackSent < 0)
            {
                cerr << "[!] [Closing] Failed to send ACK" << endl;
                return;
            }

            if (this->status == FIN_WAIT_2)
            {
                this->status == TIMED_WAIT;
            }
            else if (this->status == FIN_WAIT_1)
            {
                this->status = CLOSING;
            }
        }
        else if (flagsToByte(receivedSeg) == ACK_FLAG && receivedSeg.acknowledgementNumber == finSeg.sequenceNumber + 1)
        {
            if (this->status == CLOSING)
            {
                this->status == TIMED_WAIT;
            }
            else if (this->status == FIN_WAIT_1)
            {
                this->status == FIN_WAIT_2;
            }
        }
        else
        {
            cerr << "[!] [Closing] Invalid packet or out of order" << endl;
        }
    }

    cout << "[i] [Closing] Entering TIME_WAIT state for a brief period" << endl;
    auto timeWaitStart = chrono::steady_clock::now();
    auto timeWaitDuration = chrono::seconds(20);

    fd_set readfds;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    while (chrono::steady_clock::now() - timeWaitStart < timeWaitDuration)
    {
        FD_ZERO(&readfds);
        FD_SET(this->socket, &readfds);

        int ret = select(this->socket + 1, &readfds, nullptr, nullptr, &tv);

        if (ret > 0 && FD_ISSET(this->socket, &readfds))
        {
            // If data is available, process it
            recvBufLen = recvfrom(this->socket, recvBuf, BASE_SEGMENT_SIZE, 0,
                                    (struct sockaddr *)&clientAddress, &clientAddressLen);

            if (recvBufLen > 0)
            {
                Segment recvSeg;
                deserializeToSegment(&recvSeg, recvBuf, recvBufLen);

                if (flagsToByte(recvSeg) == FIN_FLAG)
                {
                    Segment ackSeg = ack(recvSeg.sequenceNumber + 1);
                    uint8_t *ackSegBuf = serializeSegment(&ackSeg, 0, 0);

                    cout << "[i] [Closing] Sending ACK to " << ip << ":" << port << endl;
                    ssize_t ackSent = sendto(this->socket, ackSegBuf, BASE_SEGMENT_SIZE, 0,
                                                (struct sockaddr *)&clientAddress, clientAddressLen);
                    if (ackSent < 0)
                    {
                        cerr << "[!] [Closing] Failed to send ACK" << endl;
                        return;
                    }
                }
                else
                {
                    cout << "[i] [Closing] Received stray packet during TIME_WAIT, ignoring." << endl;
                }
            }
        }

        this_thread::sleep_for(chrono::milliseconds(100));
    }

    cout << "[i] [Closing] TIME_WAIT period expired, connection is fully closed." << endl;

    this->status = CLOSED;
    cout << "[i] [Closed] Connection closed successfully with " << ip << ":" << port << endl;
    return;
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
void TCPSocket::receiveThread(sockaddr_in &serverAddress, socklen_t &serverAddressLen)
{
    while (this->isReceiving)
    {
        uint8_t recvBuffer[MAX_SEGMENT_SIZE];
        memset(recvBuffer, 0, sizeof(recvBuffer));

        ssize_t bytesRead = recvfrom(this->socket, recvBuffer, sizeof(recvBuffer), 0,
                                     (struct sockaddr *)&serverAddress, &serverAddressLen);

        if (bytesRead < 0)
        {
            // add condition to display message with [Established] or [Closing]
            if (this->isReceiving)
            {
                if (this->status == ESTABLISHED)
                {
                    cout << "[!] [Established] ";
                }
                else
                {
                    cout << "[!] [Closing] ";
                }
                cerr << "Failed to receive data" << endl;
            }
            continue;
        }
        else
        {
            vector<uint8_t> packet(recvBuffer, recvBuffer + bytesRead);
            this->packetQueue.push(packet);
        }
    }
}

void TCPSocket::processThread(sockaddr_in &serverAddress, socklen_t serverAddressLen, atomic<uint32_t> &numOfSegmentReceived)
{
    while (true)
    {
        vector<uint8_t> packet = this->packetQueue.pop();

        Segment seg;
        deserializeToSegment(&seg, packet.data(), packet.size());

        bool retransmit = false;
        if (seg.sequenceNumber != this->lfr + 1)
        {
            cout << "[!] [Established] [S=" << seg.sequenceNumber << "] [LFR=" << this->lfr << "] Segment out of order" << endl;
            retransmit = true;
        }

        if (!isValidChecksum(seg, packet.size() - BASE_SEGMENT_SIZE))
        {
            cout << "[!] [Established] [S=" << seg.sequenceNumber << "] Segment corrupt" << endl;
            retransmit = true;
        }

        if (!retransmit)
        {
            if (flagsToByte(seg) == FIN_FLAG)
            {
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(serverAddress.sin_addr), ip, INET_ADDRSTRLEN);

                uint16_t port = ntohs(serverAddress.sin_port);

                cout << "[i] [Closing] Received FIN request from " << ip << ":" << port << endl;
                handleFinHandshake(serverAddress, serverAddressLen, this->segmentHandler->getCurrentSeqNum(), seg.sequenceNumber + 1);

                this->isReceiving = false;

                break;
            }

            this->segmentHandler->appendSegmentBuffer(&seg);
            this->totalBytesRead += packet.size() - BASE_SEGMENT_SIZE;
            this->lfr = seg.sequenceNumber + packet.size() - BASE_SEGMENT_SIZE - 1;

            numOfSegmentReceived.fetch_add(1, memory_order_relaxed);
        }

        Segment ackSeg = ack(this->lfr + 1);
        ssize_t bytesSent = sendto(socket, serializeSegment(&ackSeg, 0, 0), BASE_SEGMENT_SIZE, 0,
                                    (struct sockaddr *)&serverAddress, serverAddressLen);
        if (bytesSent < 0)
        {
            cerr << "Failed to send acknowledgment" << endl;
            continue;
        }

        cout << "[i] [Established] [Seg " << numOfSegmentReceived
                << "] [A=" << ackSeg.acknowledgementNumber << "] ACK Sent" << endl;

        this->laf = this->lfr + this->rws;
    }
}

void TCPSocket::handleFinHandshake(sockaddr_in &serverAddress, socklen_t serverAddressLen, uint32_t sequenceNumber, uint32_t ackNumber)
{
    int retries = 0;

    // Send FIN-ACK to acknowledge the last FIN
    Segment sentSeg = finAck(sequenceNumber, ackNumber);
    uint8_t *sentSegBuf = serializeSegment(&sentSeg, 0, 0);

    cout << "[i] [Closing] Sending FIN-ACK..." << endl;

    this->status = LAST_ACK;
    while (retries < MAX_RETRIES)
    {
        ssize_t bytesSent = sendto(this->socket, sentSegBuf, BASE_SEGMENT_SIZE, 0,
                                   (struct sockaddr *)&serverAddress, serverAddressLen);

        if (bytesSent < 0)
        {
            cerr << "[!] [Closing] Failed to send segment" << endl;
            ++retries;
            this_thread::sleep_for(TIMEOUT_DURATION);
            continue;
        }

        // Wait for final ACK
        cout << "[i] [Closing] Waiting for final ACK..." << endl;

        bool receivedValidAck = false;

        auto startTime = chrono::steady_clock::now();
        while (chrono::steady_clock::now() - startTime < TIMEOUT_DURATION)
        {
            if (!this->packetQueue.empty())
            {
                vector<uint8_t> packet = this->packetQueue.pop();

                Segment ackSeg;
                deserializeToSegment(&ackSeg, packet.data(), packet.size());

                // Check if the received packet is a valid ACK
                if (flagsToByte(ackSeg) == ACK_FLAG && ackSeg.acknowledgementNumber == sequenceNumber + 1)
                {
                    cout << "[i] [Closing] Final ACK received. Closing connection." << endl;
                    this->isReceiving = false;
                    receivedValidAck = true;
                    return;
                }
            }

            this_thread::sleep_for(chrono::milliseconds(100));
        }

        if (receivedValidAck)
        {
            return;
        }

        // Retry on timeout or invalid packet
        cerr << "[!] [Closing] Timeout or invalid ACK. Retrying FIN..." << endl;
        ++retries;

        sentSeg = fin(sequenceNumber);
        sentSegBuf = serializeSegment(&sentSeg, 0, 0);

        this_thread::sleep_for(TIMEOUT_DURATION);
    }

    cerr << "[!] Max retries reached. Connection closing might be incomplete." << endl;
}