#ifndef socket_h
#define socket_h

#include <sys/socket.h>
#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <map>
#include "segment.hpp"
#include "segment_handler.hpp"
#include "CSPRNG.hpp"

#define time_stamp chrono::high_resolution_clock::time_point

using namespace std;

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
    LAST_ACK = 8
};

class TCPSocket
{
    // todo add tcp connection state?
private:
    /**
     * The ip address and port for the socket instance
     * Not to be confused with ip address and port of the connected connection
     */
    string ip;
    int32_t port;

    /**
     * Socket descriptor
     */
    int32_t socket;

    SegmentHandler *segmentHandler;

    TCPStatusEnum status;

    CSPRNG *rand;

    uint32_t sws = 0; // sent window size
    uint32_t lar = 0; // last ack received
    uint32_t lfs = 0; // last frame sent

    uint32_t rws = 0; // received window size
    uint32_t lfr = 0; // last frame received
    uint32_t laf = 0; // largest acceptable frame

    vector<Segment *> window;
    // cuman buat server

    mutex serverLock;
    void listenACK(string ip, int32_t port);

public:
    TCPSocket(string ip, int32_t port);
    ~TCPSocket();

    TCPStatusEnum getStatus();

    pair<string, int32_t> listen();

    /**
     * This function sends a connection request to the broadcast address of the network
     */
    string connect(string broadcastAddr, int32_t port);

    void send(string ip, int32_t port, void *dataStream, uint32_t dataSize);
    int32_t recv(void *buffer, uint32_t length);
    void close(string ip, int32_t port);
};

#endif