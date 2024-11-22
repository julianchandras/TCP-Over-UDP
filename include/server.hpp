#ifndef SERVER_H
#define SERVER_H

#include "node.hpp"

class Server : public Node
{
public:
    Server(string ip, int32_t port);

    void run();
    void handleMessage(void *buffer) override;
};

#endif