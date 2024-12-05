#include "server.hpp"
#include "segment_handler.hpp"
#include "utils.hpp"
#include <iostream>
#include <arpa/inet.h>

using namespace std;

Server::Server(string ip, int32_t port) : Node(ip, port) {}

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

    size_t dataSize = input.size();
    uint8_t *dataStream = (uint8_t *)malloc(dataSize);

    memcpy(dataStream, input.c_str(), dataSize);

    auto [clientIp, clientPort] = this->connection->listen();
    
    this->connection->send(clientIp, clientPort, dataStream, dataSize);

    // segHand.sendData(this->connection, input);

    // // putting input into a segment aka adding tcp header
    // Segment testSeg = ack(69, 69);
    // size_t payloadSize = input.size();
    // testSeg.payload = (uint8_t *)malloc(payloadSize);
    // if (testSeg.payload == nullptr)
    // {
    //     cerr << "Error: Memory allocation failed for payload!" << endl;
    //     return;
    // }
    // memcpy(testSeg.payload, input.c_str(), payloadSize);
    // cout << "[+] Payload inserted" << endl;

    // // serialize aka jadiin segment mjd bytes
    // uint8_t *buffer = serializeSegment(&testSeg, 0, payloadSize);
    // cout << "[+] Segment serialized " << endl;

    // // Try to send a UDP Datagram
    // size_t total_size = BASE_SEGMENT_SIZE + 0 + payloadSize; // option len dianggap 0, idk option buat apa tbh
    // this->connection->send("127.0.0.1", 5679, buffer, total_size);
    // cout << "[+] User input has been successfully received." << endl;
    // cout << "[+] Data sent to client." << endl;
    // free(buffer);
}

void Server::handleMessage(void *buffer)
{
}