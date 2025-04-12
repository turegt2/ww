#include "../include/client.h"
#include <iostream>
#include <chrono>
#include <vector>

using namespace std;

string encryptMessage(const string& input, const string& key) {
    string output = input;
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = (input[i] ^ key[i % key.size()]) + 1;
    }
    return output;
}

string decryptMessage(const string& input, const string& key) {
    string output = input;
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = (input[i] - 1) ^ key[i % key.size()];
    }
    return output;
}

void receiveMessages(SOCKET socket, const string& key) {
    char buffer[2048];
    while (true) {
        int bytesReceived = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            cout << "Server disconnected.\n";
            exit(0);
        }
        buffer[bytesReceived] = '\0';
        string message(buffer);

        if (message.substr(0, 8) == "[SYSTEM]") {
            cout << message.substr(8) << endl;
        } 
        else if (message.substr(0, 11) == "[ENCRYPTED]") {
            string decrypted = decryptMessage(message.substr(11), key);
            size_t colonPos = decrypted.find(":");
            if (colonPos != string::npos) {
                string sender = decrypted.substr(0, colonPos);
                string msg = decrypted.substr(colonPos + 2);
                cout << "Received from " << sender << ": " << msg << endl;
            } else {
                cout << "Received " << decryptMessage(message, key) << endl;
            }
        }
    }
}

bool connectToServer(SOCKET& clientSocket, sockaddr_in& serverAddr) {
    unsigned long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(clientSocket, &set);
        timeval timeout{ 2, 0 };

        for (int i = 5; i >= 0; --i) {
            clientSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
                cout << "Server not found. Countdown: " << i << " \r";
                cout.flush();
                this_thread::sleep_for(chrono::seconds(1));
            } else {
                cout << "                              " << " \r";
                cout.flush();
                return true;
            }
        }
        return false;
    }
    return true;
}