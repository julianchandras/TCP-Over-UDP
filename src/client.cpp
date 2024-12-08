#include "client.hpp"
#include "utils.hpp"
#include <iostream>

using namespace std;

Client::Client(const string &ip, int32_t port) : Node(ip, port) {}

void Client::run()
{
    string broadcastAddr;
    int32_t serverPort;

    auto interfaces = Node::getNetworkInterfaces();

    if (interfaces.size() > 2)
    {
        broadcastAddr = interfaces.at(2).broadcast;
    }
    else
    {
        broadcastAddr = interfaces.at(0).broadcast;
    }

    cout << "[?] Input the server program's port: ";
    cin >> serverPort;

    this->serverPort = serverPort;
    this->serverIp = this->connection->connect(broadcastAddr, this->serverPort);

    uint8_t buffer[2000];
    memset(buffer, 0, sizeof(buffer));
    int bytesRead = this->connection->recv(buffer, sizeof(buffer));

    cout << "[i] Message received: " << buffer << endl;
}
