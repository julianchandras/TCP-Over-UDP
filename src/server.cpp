#include "server.hpp"
#include "segment_handler.hpp"
#include "utils.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdint>
#include <cstdlib>

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
    cin.ignore(); // To ignore the newline character left in the input buffer

    uint8_t *dataStream = nullptr; // Initialize to null
    size_t dataSize = 0;

    if (choice == 1)
    {
        cout << "[?] User input mode chosen, please enter your input: ";
        string input;
        getline(cin, input);

        // Allocate and copy input into the data stream
        dataSize = input.size();
        dataStream = (uint8_t *)malloc(dataSize);
        memcpy(dataStream, input.c_str(), dataSize);
    }
    else if (choice == 2)
    {
        cout << "[?] File input mode chosen, please enter the file path: ";
        string filePath;
        getline(cin, filePath);

        ifstream file(filePath, ios::binary);
        if (!file.is_open())
        {
            cerr << "[!] Error: Could not open file " << filePath << endl;
            return;
        }

        size_t pos = filePath.find_last_of("/\\");
        string fileName = (pos == string::npos) ? filePath : filePath.substr(pos + 1);
        vector<uint8_t> fileContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close();
        uint32_t fileNameLength = fileName.size();
        dataSize = sizeof(fileNameLength) + fileNameLength + fileContent.size();
        dataStream = (uint8_t *)malloc(dataSize);
        memcpy(dataStream, &fileNameLength, sizeof(fileNameLength));
        memcpy(dataStream + sizeof(fileNameLength), fileName.c_str(), fileNameLength);
        memcpy(dataStream + sizeof(fileNameLength) + fileNameLength, fileContent.data(), fileContent.size());
    }
    else
    {
        cerr << "[!] Invalid choice, exiting..." << endl;
        return;
    }

    // Send the data stream to the client
    auto [clientIp, clientPort] = this->connection->listen();
    this->connection->send(clientIp, clientPort, dataStream, dataSize);

    // Free the allocated memory
    free(dataStream);
}
void Server::handleMessage(void *buffer)
{
}