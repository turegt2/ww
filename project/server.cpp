#include "../include/server.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

vector<SOCKET> clients;
map<SOCKET, string> clientNames;
map<SOCKET, bool> admins;
mutex clientsMutex;

void User::broadcastMessage(const string& message, SOCKET sender,const vector<string>& exclude) {
    lock_guard<mutex> lock(clientsMutex);
    for (auto& client : clients) {
        bool isExcluded = find(exclude.begin(), exclude.end(),clientNames[client]) != exclude.end();
        if (client != sender && !isExcluded) {
            send(client, message.c_str(), message.size(), 0);
        }
    }
}

void User::broadcastMessage(const string& message, const string& recipientName) {
    lock_guard<mutex> lock(clientsMutex);
    for (auto& [client, name] : clientNames) {
        if (name == recipientName) {
            send(client, message.c_str(), message.size(), 0);
            return;
        }
    }
    cout << "Recipient not found: " << recipientName << endl;
}

void User::sendToMultiple(const string& message, const vector<string>& recipients, SOCKET sender) {
    lock_guard<mutex> lock(clientsMutex);
    for (auto& [client, name] : clientNames) {
        bool isRecipient = find(recipients.begin(), recipients.end(), name) != recipients.end();
        if (client != sender && isRecipient) {
            send(client, message.c_str(), message.size(), 0);
        }
    }
}

void Admin::disconnectClient(const string& name) {
    lock_guard<mutex> lock(clientsMutex);
    for (auto it = clientNames.begin(); it != clientNames.end();) {
        if (it->second == name) {
            closesocket(it->first);
            clients.erase(remove(clients.begin(), clients.end(), it->first), clients.end());
            it = clientNames.erase(it);
            cout << "[ADMIN] Client " << name << " disconnected.\n";
        } else {
            ++it;
        }
    }
}

void Admin::disconnectAll(SOCKET serverSocket) {
    lock_guard<mutex> lock(clientsMutex);
    for (auto& client : clients) {
        closesocket(client);
    }
    clients.clear();
    clientNames.clear();
    closesocket(serverSocket);
    cout << "[ADMIN] All clients disconnected. Server shutdown.\n";
    exit(0);
}

void handleClient(SOCKET clientSocket, SOCKET serverSocket) {
    char nameBuffer[1024];
    int bytesReceived = recv(clientSocket, nameBuffer, sizeof(nameBuffer) - 1, 0);
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }
    nameBuffer[bytesReceived] = '\0';
    string clientName = nameBuffer;

    {
        lock_guard<mutex> lock(clientsMutex);

        for (auto& [_, name] : clientNames) {
            if (name == clientName) {
                string error = "[SYSTEM]Name already exists!";
                send(clientSocket, error.c_str(), error.size(), 0);
                closesocket(clientSocket);
                return;
            }
        }

        clientNames[clientSocket] = clientName;
        cout << "New connection: " << clientName << endl;
    }

    char buffer[2048];
    while (true) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) break;

        buffer[bytesReceived] = '\0';
        string message(buffer);

        if (message.substr(0, 4) == "off ") {
            if (admins.count(clientSocket)) {
                string target = message.substr(4);
                if (target == "all") {
                    Admin::disconnectAll(serverSocket);
                } else {
                    Admin::disconnectClient(target);
                }
            }
        }
        else if (message.substr(0, 10) == "reg_admin ") {
            stringstream ss(message.substr(10));
            string login, password;
            ss >> login >> password;
        
            if (login.empty() || password.empty()) {
                string response = "[SYSTEM][Server] Invalid format. Use: reg_admin login password";
                send(clientSocket, response.c_str(), response.size(), 0);
                continue;
            }
        
            ifstream file("admins.txt");
            bool found = false;
            string line;
            while (getline(file, line)) {
                stringstream lineStream(line);
                string fileLogin, filePassword;
                if (lineStream >> fileLogin >> filePassword) {
                    if (fileLogin == login && filePassword == password) {
                        found = true;
                        break;
                    }
                }
            }
            file.close();
        
            if (found) {
                admins[clientSocket] = true;
                string response = "[SYSTEM][Server] Admin rights granted";
                send(clientSocket, response.c_str(), response.size(), 0);
                cout << "[ADMIN] Administrator connected: " << clientName << endl;
            } else {
                string response = "[SYSTEM][Server] Access denied. Invalid credentials";
                send(clientSocket, response.c_str(), response.size(), 0);
            }
        }
        
        else if (message.substr(0, 3) == "to ") {
            size_t pos = message.find(":");
            if (pos != string::npos) {
                string recipientsStr = message.substr(3, pos - 3);
                vector<string> recipients;
                stringstream ss(recipientsStr);
                string recipient;
                while (ss >> recipient) {
                    recipients.push_back(recipient);
                }
                string content = "[ENCRYPTED]" + message.substr(pos + 2);
                User::sendToMultiple(content, recipients, clientSocket);
            }
        }
        else if (message.substr(0, 8) == "without ") {
            size_t pos = message.find(":");
            if (pos != string::npos) {
                string excludeStr = message.substr(7, pos - 7);
                vector<string> exclude;
                stringstream ss(excludeStr);
                string name;
                while (ss >> name) {
                    exclude.push_back(name);
                }
                string content = "[ENCRYPTED]" + message.substr(pos + 2);
                User::broadcastMessage(content, clientSocket, exclude);
            }
        }
        else {
            string content = "[ENCRYPTED]" + message;
            User::broadcastMessage(content, clientSocket);
        }
    }

    {
        lock_guard<mutex> lock(clientsMutex);
        clients.erase(remove(clients.begin(), clients.end(), clientSocket), clients.end());
        clientNames.erase(clientSocket);
        if (admins.count(clientSocket)) admins.erase(clientSocket);
    }
    closesocket(clientSocket);
}

void startServer() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    cout << "Server is running. Waiting for connections...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) continue;

        {
            lock_guard<mutex> lock(clientsMutex);
            clients.push_back(clientSocket);
        }
        
        thread(handleClient, clientSocket, serverSocket).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
}