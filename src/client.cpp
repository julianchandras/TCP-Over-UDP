#include "client.hpp"
#include <iostream>

Client::Client(string ip, int32_t port) : Node(ip, port) {}

void Client::run() {
    string serverHost;
    int32_t serverPort;

    cout << "[?] Input the server program's host: ";
    cin >> serverHost;
    // Probably the above code should not exist since the client look for server through broadcast

    cout << "[?] Input the server program's port: ";
    cin >> serverPort;

    this->serverIp = serverHost;
    this->serverPort = serverPort;

    // cout << "[+] Trying to contact the sender at " << this->serverIp << ":" << this->serverPort << endl;
    // TODO: handshake
    // this->connection->connect(this->serverPort)

    // UDP Trial
    
    char buffer[1024];
    int bytesRead = this->connection->recv(buffer, sizeof(buffer));
    buffer[bytesRead] = '\0';

    cout << "[+] Received from server: " << buffer << endl;
}

void Client::handleMessage(void *buffer) {

}