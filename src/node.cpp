#include <iostream>
#include "client.hpp"
#include "server.hpp"

using namespace std;

Node::Node(string ip, int32_t port)
{
    this->connection = new TCPSocket(ip, port);
}

// string Node::resolveHostname(const string& hostname)
// {
    
// }

// vector<pair<string, string>> Node::getLocalInterfaces()
// {
    
// }

// string Node::calculateBroadcastAddress(const string& ip, const string& mask)
// {
    
// }

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        cerr << "Usage: node [host] [port]" << endl;
        return 1;
    }

    string host = argv[1];
    int port = stoi(argv[2]);

    cout << "[i] Node started at " << host << ":" << port << endl;
    cout << "[?] Please choose the operating mode" << endl;
    cout << "[?] 1. Sender" << endl;
    cout << "[?] 2. Receiver" << endl;
    cout << "[?] Input: ";

    int choice;
    cin >> choice;

    Node* node = nullptr;

    if (choice == 1)
    {
        node = new Server(host, port);
        cout << "[+] Node is now a sender" << endl;
    } else if (choice == 2)
    {
        node = new Client(host, port);
        cout << "[+] Node is now a receiver" << endl;
    }

    try
    {
        node->run();
        return 0;
    }
    catch (const std::exception& e)
    {
        cerr << "[!] ERROR: " << e.what() << endl;
        return -1;
    }

}

Node::~Node()
{
    delete connection;
}