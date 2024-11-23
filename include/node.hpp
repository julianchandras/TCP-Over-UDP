#ifndef node_h
#define node_h

#include "socket.hpp"

class Server;
class Client;

/**
 * Abstract class.
 *
 * This is the base class for Server and Client class.
 */
class Node
{
protected:
    TCPSocket *connection;

public:
    Node(string ip, int32_t port);
    ~Node();

    virtual void run() = 0;
};

#endif