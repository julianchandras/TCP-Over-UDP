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

        // Extract the file name and extension
        size_t pos = filePath.find_last_of("/\\");
        string fileName = (pos == string::npos) ? filePath : filePath.substr(pos + 1);
        size_t extPos = fileName.find_last_of(".");
        string fileExtension = (extPos == string::npos) ? "" : fileName.substr(extPos + 1);
        string nameWithoutExtension = (extPos == string::npos) ? fileName : fileName.substr(0, extPos);

        // Read the file content
        vector<uint8_t> fileContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        file.close();

        // Calculate sizes
        uint32_t nameLength = nameWithoutExtension.size();
        uint32_t extLength = fileExtension.size();
        size_t fixedPayloadSize = sizeof(nameLength) + sizeof(extLength);
        dataSize = fixedPayloadSize + nameLength + extLength + fileContent.size();

        // Allocate the buffer
        dataStream = (uint8_t *)malloc(dataSize);

        // Fill the buffer
        size_t offset = 0;

        // Copy name length
        memcpy(dataStream + offset, &nameLength, sizeof(nameLength));
        offset += sizeof(nameLength);

        // Copy extension length
        memcpy(dataStream + offset, &extLength, sizeof(extLength));
        offset += sizeof(extLength);

        // Copy name
        memcpy(dataStream + offset, nameWithoutExtension.c_str(), nameLength);
        offset += nameLength;

        // Copy extension
        memcpy(dataStream + offset, fileExtension.c_str(), extLength);
        offset += extLength;

        // Copy file content
        memcpy(dataStream + offset, fileContent.data(), fileContent.size());
    }
    else
    {
        cerr << "[!] Invalid choice, exiting..." << endl;
        return;
    }

    // Send the data stream to the client
    auto [clientIp, clientPort] = this->connection->listen();
    this->connection->send(clientIp, clientPort, dataStream, dataSize);
    this->connection->close(clientIp, clientPort);
    free(dataStream);
}
