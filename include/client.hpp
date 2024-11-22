#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include "node.hpp"

using namespace std;

class Client : public Node
{
private:
    string server_id;
    int server_port;

public:
    Client(string server_id, int server_port);
    void run();
    void handleMessage(void *buffer) override;
};

#endif