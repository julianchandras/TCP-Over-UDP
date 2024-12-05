#include "client.hpp"
#include "utils.hpp"
#include <iostream>

Client::Client(string ip, int32_t port) : Node(ip, port) {}

void Client::run()
{
    string serverHost;
    int32_t serverPort;

    cout << "[?] Input the server program's host: ";
    cin >> serverHost;
    // Probably the above code should not exist since the client look for server through broadcast

    cout << "[?] Input the server program's port: ";
    cin >> serverPort;

    this->serverIp = serverHost;
    this->serverPort = serverPort;

    this->connection->connect(this->serverIp, this->serverPort);

    uint8_t buffer[1460];
    int bytesRead = this->connection->recv(buffer, sizeof(buffer));

    // suppose a random segment is available
    Segment testSegRec = initializeSegment();
    deserializeToSegment(&testSegRec, buffer, bytesRead);
    printSegment(testSegRec, 200);
}

void Client::handleMessage(void *buffer)
{
}