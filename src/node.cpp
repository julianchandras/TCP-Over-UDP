#include "client.hpp"
#include "server.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <ifaddrs.h>

using namespace std;

Node::Node(const string &ip, int32_t port)
{
    this->connection = new TCPSocket(ip, port);
}

string Node::resolveHostname(const string& hostname)
{
    return "";
}

vector<NetworkInterface> Node::getNetworkInterfaces()
{
    vector<NetworkInterface> interfaces;
    struct ifaddrs *ifap, *ifa;
    char ipstr[INET6_ADDRSTRLEN];

    if (getifaddrs(&ifap) == -1) {
        throw std::runtime_error("Could not get interface addresses");
    }

    for (ifa = ifap; ifa; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)
        {
            NetworkInterface iface;

            iface.name = ifa->ifa_name;

            // IP Address
            struct sockaddr_in* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            inet_ntop(AF_INET, &(sa->sin_addr), ipstr, sizeof(ipstr));
            iface.ip = ipstr;

            // Broadcast Address
            struct sockaddr_in* broadaddr_sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_broadaddr);
            inet_ntop(AF_INET, &(broadaddr_sa->sin_addr), ipstr, sizeof(ipstr));
            iface.broadcast = ipstr;

            interfaces.push_back(iface);
        }
    }

    freeifaddrs(ifap);
    return interfaces;
}

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
    catch (const exception& e)
    {
        cerr << "[!] ERROR: " << e.what() << endl;
        return -1;
    }

}

Node::~Node()
{
    delete connection;
}