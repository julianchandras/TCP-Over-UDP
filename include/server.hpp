#ifndef SERVER_H
#define SERVER_H

#include "node.hpp"

class Server : public Node
{
public:
    Server(const std::string &ip, int32_t port);
    void run() override;
};

#endif
