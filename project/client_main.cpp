#include "../include/client.h"
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")
#define PORT 5052

using namespace std;

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /*string serverIP;
    cout << "Enter server IP: ";
    getline(cin, serverIP);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
    172.20.10.7*/

    serverAddr.sin_port = htons(PORT);
    


    

    if (!connectToServer(clientSocket, serverAddr)) {
        cout << "\nConnection failed. Exiting...\n";
        return 1;
    }

    string key, name;
    cout << "Enter secret key: ";
    getline(cin, key);
    cout << "Enter your name: ";
    getline(cin, name);

    send(clientSocket, name.c_str(), name.size(), 0);

    thread(receiveMessages, clientSocket, key).detach();

    string message;
    while (true) {
        getline(cin, message);

        if (message.substr(0, 3) == "to " || message.substr(0, 8) == "without ") {
            size_t pos = message.find(":");
            if (pos != string::npos) {
                string prefix = message.substr(0, pos + 2);
                string content = name + ": " + message.substr(pos + 2);
                string encryptedContent = encryptMessage(content, key);
                send(clientSocket, (prefix + encryptedContent).c_str(), prefix.size() + encryptedContent.size(), 0);
            }
        } 
        else if (message.substr(0, 4) == "off " || message.substr(0, 10) == "reg_admin ") {
            send(clientSocket, message.c_str(), message.size(), 0);
        } 
        else {
            string fullMessage = name + ": " + message;
            string encrypted = encryptMessage(fullMessage, key);
            send(clientSocket, encrypted.c_str(), encrypted.size(), 0);
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}