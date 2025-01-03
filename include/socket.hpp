#ifndef socket_h
#define socket_h

#include <sys/socket.h>
#include <string>
#include <functional>
#include <vector>
#include <mutex>
#include <chrono>
#include <arpa/inet.h>
#include <atomic>
#include <fcntl.h>
#include <set>
#include "segment.hpp"
#include "segment_handler.hpp"
#include "CSPRNG.hpp"
#include "thread_safe_queue.hpp"

#define time_stamp std::chrono::high_resolution_clock::time_point

const uint8_t DEFAULT_WINDOW_SIZE = 7;
const uint16_t MAX_RETRIES = 5;

// for references
// https://maxnilz.com/docs/004-network/003-tcp-connection-state/
// You can use a different state enum if you'd like
enum TCPStatusEnum
{
    LISTEN = 0,
    SYN_SENT = 1,
    SYN_RECEIVED = 2,
    ESTABLISHED = 3,
    FIN_WAIT_1 = 4,
    FIN_WAIT_2 = 5,
    CLOSE_WAIT = 6,
    CLOSING = 7,
    LAST_ACK = 8,
    TIMED_WAIT = 9,
    CLOSED = 10,
};

class TCPSocket
{
    // todo add tcp connection state?
private:
    /**
     * The ip address and port for the socket instance
     * Not to be confused with ip address and port of the connected connection
     */
    std::string ip;
    int32_t port;

    /**
     * Socket descriptor
     */
    int32_t socket;

    SegmentHandler *segmentHandler;

    TCPStatusEnum status;

    CSPRNG *rand;

    uint32_t sws = DEFAULT_WINDOW_SIZE * MAX_PAYLOAD_SIZE; // sent window size
    uint32_t lar = 0;                                      // last ack received
    uint32_t lfs = 0;                                      // last frame sent

    uint32_t rws = DEFAULT_WINDOW_SIZE * MAX_PAYLOAD_SIZE; // received window size
    uint32_t lfr = 0;                                      // last frame received
    uint32_t laf = 0;                                      // largest acceptable frame

    std::vector<Segment *> window;

    // buat send()
    std::mutex serverLock;
    bool terminateACK;
    void listenACK(const std::string &ip, int32_t port);

    // buat recv()
    bool isReceiving;
    uint32_t totalBytesRead;
    ThreadSafeQueue<std::vector<uint8_t>> packetQueue;

    void receiveThread(sockaddr_in &serverAddress, socklen_t &serverAddressLen);
    void processThread(sockaddr_in &serverAddress, socklen_t serverAddressLen,
                       std::atomic<uint32_t> &numOfSegmentReceived);
    void handleFinHandshake(sockaddr_in &serverAddress, socklen_t serverAddressLen, uint32_t sequenceNumber, uint32_t ackNumber);

public:
    TCPSocket(const std::string &ip, int32_t port);
    ~TCPSocket();

    std::pair<std::string, int32_t> listen();

    /**
     * Sends a connection request to the broadcast address of the network
     */
    std::string connect(const std::string &broadcastAddr, int32_t port);

    void send(const std::string &ip, int32_t port, void *dataStream, uint32_t dataSize);
    int32_t recv(std::vector<uint8_t> &dataStream);
    void close(const std::string &ip, int32_t port);
};

#endif
