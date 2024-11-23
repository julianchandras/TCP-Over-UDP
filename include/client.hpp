#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "node.hpp"

using namespace std;

class Client : public Node
{
private:
    string serverIp;
    int32_t serverPort;

    void handleMessage(void *buffer);

public:
    Client(string ip, int32_t port);

    void run();
};

#endif