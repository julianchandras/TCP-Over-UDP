#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "node.hpp"

class Client : public Node
{
private:
    std::string serverIp;
    int32_t serverPort;

    void handleMessage(void *buffer);

public:
    Client(const std::string &ip, int32_t port);

    void run() override;
};

#endif
