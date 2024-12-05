#ifndef node_h
#define node_h

#include "socket.hpp"
#include <string>
#include <vector>

class Server;
class Client;

/**
 * Abstract class.
 *
 * This is the base class for Server and Client class.
 */
class Node
{
// private:
//     static string resolveHostname(const string& hostname);
//     static vector<pair<string, string>> getLocalInterfaces();
//     static string calculateBroadcastAddress(const string& ip, const string& mask);

protected:
    TCPSocket *connection;

public:
    Node(string ip, int32_t port);
    ~Node();

    virtual void run() = 0;
};

#endif