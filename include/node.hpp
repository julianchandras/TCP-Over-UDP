#ifndef node_h
#define node_h

#include "socket.hpp"
#include <string>
#include <vector>

class Server;
class Client;

struct NetworkInterface
{
    std::string name;
    std::string ip;
    std::string broadcast;
};

void printNetworkInterfaces(const std::vector<NetworkInterface> &interfaces)
{
    for (const auto &iface : interfaces)
    {
        std::cout << "Name: " << iface.name << std::endl;
        std::cout << "IP: " << iface.ip << std::endl;
        std::cout << "Broadcast: " << iface.broadcast << std::endl;
        std::cout << "---------------------------" << std::endl;
    }
}

/**
 * Abstract class.
 *
 * This is the base class for Server and Client class.
 */
class Node
{
private:
    static std::string resolveHostname(const std::string &hostname);

protected:
    TCPSocket *connection;

    static std::vector<NetworkInterface> getNetworkInterfaces();

public:
    Node(const std::string &ip, int32_t port);
    virtual ~Node();

    virtual void run() = 0;
};

#endif
