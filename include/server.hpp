#ifndef SERVER_H
#define SERVER_H


#include "node.hpp"

class Server : public Node
{
private:
    void handleMessage(void *buffer);

public:
    Server(string ip, int32_t port);
    void run();
};

#endif