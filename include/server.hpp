#ifndef SERVER_H
#define SERVER_H

#include "node.hpp"

class Server : public Node
{
public:
    void run();
    void handleMessage(void *buffer) override;
};

#endif