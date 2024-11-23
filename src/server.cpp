#include "server.hpp"
#include <iostream>

using namespace std;

Server::Server(string ip, int32_t port) : Node(ip, port) {}

void Server::run() {
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

    cout << "[+] User input has been successfully received." << endl;

    // this->connection->listen();
    // the above line is important since the server itself must listen to handshake request

    // Try to send a UDP Datagram
    this->connection->send("127.0.0.1", 5679, (void*)input.data(), input.size());
    cout << "[+] Data sent to client." << endl;
}

void Server::handleMessage(void *buffer) {

}