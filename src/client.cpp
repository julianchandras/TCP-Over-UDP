#include "client.hpp"
#include "utils.hpp"
#include <iostream>
#include <fstream>

using namespace std;

Client::Client(const string &ip, int32_t port) : Node(ip, port) {}

void printNetworkInterfaces(const std::vector<NetworkInterface> &interfaces)
{
    for (const auto &iface : interfaces)
    {
        std::cout << "Name: " << iface.name << std::endl;
        std::cout << "IP: " << iface.ip << std::endl;
        std::cout << "Broadcast: " << iface.broadcast << std::endl;
        std::cout << "---------------------------" << std::endl;
    }
}

void Client::run()
{
    string broadcastAddr;
    int32_t serverPort;

    auto interfaces = Node::getNetworkInterfaces();
    printNetworkInterfaces(interfaces);
    if (interfaces.size() > 1)
    {
        broadcastAddr = interfaces.at(1).broadcast;
    }
    else
    {
        broadcastAddr = interfaces.at(0).broadcast;
    }

    cout << "[?] Input the server program's port: ";
    cin >> serverPort;

    this->serverPort = serverPort;
    this->serverIp = this->connection->connect(broadcastAddr, this->serverPort);

    vector<uint8_t> buffer;
    int dataLength = this->connection->recv(buffer);
    if (dataLength <= 0)
    {
        cerr << "[!] No data received or connection error." << endl;
        return;
    }

    size_t offset = 0;

    // Extract file name length
    uint32_t nameLength;
    memcpy(&nameLength, buffer.data() + offset, sizeof(nameLength));
    offset += sizeof(nameLength);

    uint32_t extLength;
    memcpy(&extLength, buffer.data() + offset, sizeof(extLength));
    offset += sizeof(extLength);

    if (nameLength == 0)
    {
        cout << "[i] Message received: "
             << string(reinterpret_cast<char *>(buffer.data() + offset), dataLength - offset)
             << endl;
        cout << "[i] Message length: " << dataLength - offset << endl;
        return;
    }

    string fileName(reinterpret_cast<char *>(buffer.data() + offset), nameLength);
    offset += nameLength;

    string fileExtension(reinterpret_cast<char *>(buffer.data() + offset), extLength);
    offset += extLength;

    // Extract file content
    size_t contentLength = dataLength - offset;
    vector<uint8_t> fileContent(buffer.begin() + offset, buffer.begin() + offset + contentLength);

    // Construct full file name
    string fullFileName = fileName + "." + fileExtension;

    // Save the file
    ofstream outputFile(fullFileName, ios::binary);
    if (!outputFile.is_open())
    {
        cerr << "[!] Could not create file: " << fullFileName << endl;
        return;
    }
    outputFile.write(reinterpret_cast<const char *>(fileContent.data()), fileContent.size());
    outputFile.close();

    cout << "[i] File saved as: " << fullFileName << endl;
    cout << "[i] File size: " << fileContent.size() << " bytes" << endl;
}
