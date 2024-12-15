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

    uint8_t *buffer = (uint8_t *)malloc(2000 * sizeof(uint8_t));
    if (buffer == nullptr)
    {
        cerr << "[!] Memory allocation failed!" << endl;
        return;
    }

    int bytesRead = this->connection->recv(buffer, 2000);
    if (bytesRead <= 0)
    {
        cerr << "[!] No data received or connection error." << endl;
        free(buffer);
        return;
    }

    size_t offset = 0;

    // Extract file name length
    uint32_t nameLength;
    memcpy(&nameLength, buffer + offset, sizeof(nameLength));
    offset += sizeof(nameLength);

    uint32_t extLength;
    memcpy(&extLength, buffer + offset, sizeof(extLength));
    offset += sizeof(extLength);

    if (nameLength == 0)
    {
        cout << "[i] Message received: " 
             << string(reinterpret_cast<char *>(buffer + offset), bytesRead - offset) 
             << endl;
        cout << "[i] Message length: " << bytesRead - offset << endl;
        free(buffer);
        return;
    }

    string fileName(reinterpret_cast<char *>(buffer + offset), nameLength);
    offset += nameLength;

    string fileExtension(reinterpret_cast<char *>(buffer + offset), extLength);
    offset += extLength;

    // Extract file content
    size_t contentLength = bytesRead - offset;
    vector<uint8_t> fileContent(buffer + offset, buffer + offset + contentLength);

    // Construct full file name
    string fullFileName = fileName + "." + fileExtension;

    // Save the file
    ofstream outputFile(fullFileName, ios::binary);
    if (!outputFile.is_open())
    {
        cerr << "[!] Could not create file: " << fullFileName << endl;
        free(buffer);
        return;
    }
    outputFile.write(reinterpret_cast<const char *>(fileContent.data()), fileContent.size());
    outputFile.close();

    cout << "[i] File saved as: " << fullFileName << endl;
    cout << "[i] File size: " << fileContent.size() << " bytes" << endl;

    free(buffer);
}