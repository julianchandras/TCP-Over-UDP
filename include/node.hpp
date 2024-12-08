#ifndef node_h
#define node_h

#include "socket.hpp"
#include <string>
#include <vector>

class Server;
class Client;

using namespace std;

struct NetworkInterface
{
    string name;
    string ip;
    string broadcast;
};

/**
 * Abstract class.
 *
 * This is the base class for Server and Client class.
 */
class Node
{
private:
    static string resolveHostname(const string& hostname);

protected:
    TCPSocket *connection;

    static vector<NetworkInterface> getNetworkInterfaces();

public:
    Node(string ip, int32_t port);
    ~Node();

    virtual void run() = 0;
};

#endif