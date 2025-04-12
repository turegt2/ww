#pragma once
#include <iostream>
#include <winsock2.h>
#include <vector>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <algorithm>
#include <fstream>
#include <sstream>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define PORT 5052

extern vector<SOCKET> clients;
extern map<SOCKET, string> clientNames;
extern map<SOCKET, bool> admins;
extern mutex clientsMutex;

class User {
    public:
        static void broadcastMessage(const string& message, SOCKET sender,
                                   const vector<string>& exclude = {});
        static void broadcastMessage(const string& message, const string& recipientName);
        static void sendToMultiple(const string& message,
                                 const vector<string>& recipients,
                                 SOCKET sender);
};

class Admin : public User {
public:
    static void disconnectClient(const string& name);
    static void disconnectAll(SOCKET serverSocket);
};

void handleClient(SOCKET clientSocket, SOCKET serverSocket);
void startServer();