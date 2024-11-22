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

public:
    Client(string ip, int32_t port);

    void run();
    void handleMessage(void *buffer) override;
};

#endif