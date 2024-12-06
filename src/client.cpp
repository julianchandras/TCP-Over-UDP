#include "client.hpp"
#include "utils.hpp"
#include <iostream>

Client::Client(string ip, int32_t port) : Node(ip, port) {}

void Client::run()
{
    string broadcastAddr;
    int32_t serverPort;

    auto interfaces = Node::getNetworkInterfaces();
    for (const auto& interface : interfaces)
    {
        cout << "Interface: " << interface.name << endl;
        cout << "  IP: " << interface.ip << endl;
        cout << "  Broadcast: " << interface.broadcast << endl;
    }

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

    uint8_t buffer[1460];
    memset(buffer, 0, sizeof(buffer));
    int bytesRead = this->connection->recv(buffer, sizeof(buffer));

    cout << "[i] Message received: " << buffer << endl;
}

void Client::handleMessage(void *buffer)
{
}