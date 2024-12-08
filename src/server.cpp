#include "server.hpp"
#include "segment_handler.hpp"
#include "utils.hpp"
#include <iostream>
#include <arpa/inet.h>

using namespace std;

Server::Server(const string &ip, int32_t port) : Node(ip, port) {}

void Server::run()
{
    cout << "[?] Please choose the sending mode" << endl;
    cout << "[?] 1. User input" << endl;
    cout << "[?] 2. File input" << endl;
    cout << "[?] Input: ";

    int choice;
    cin >> choice;

    // TODO: separate user/file input logic
    string input;
    cout << "[?] User input mode chosen, please enter your input: ";
    cin.ignore();
    getline(cin, input);

    auto [clientIp, clientPort] = this->connection->listen();

    size_t dataSize = input.size();
    uint8_t *dataStream = (uint8_t *)malloc(dataSize);

    memcpy(dataStream, input.c_str(), dataSize);
    
    this->connection->send(clientIp, clientPort, dataStream, dataSize);
    this->connection->close(clientIp, clientPort);
}
