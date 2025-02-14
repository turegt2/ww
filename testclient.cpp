#include <iostream>
#include <winsock2.h>
#include <thread>
#include <string>
#include <chrono>
#pragma comment(lib, "ws2_32.lib")
#define PORT 5052

using namespace std;


string shifruem(const string& input, const string& key) {
    string output = input;
    for (int i = 0; i < input.size(); i++) {                //шифруем сообщение ключом
        output[i] = input[i] ^ key[i % key.size()];
    }
    return output;
}

//получение сообщений от сервера
void receiveMessages(SOCKET socket, const string& key) {
    char buffer[2048];
    int bytesReceived = 0;

    while (true) {
        bytesReceived = recv(socket, buffer, sizeof(buffer) - 1, 0); // число считанных байтов, когда отключается сокет, возвращает текст
        if (bytesReceived <= 0) {
            cout << "server disconnected." << endl;
            break;
        }
        buffer[bytesReceived] = '\0'; 
        string decryptedMessage = shifruem(buffer, key);
        
        
        cout << "Received: " << decryptedMessage << "  " << endl; 
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    string key;

    cout << "Input secret key word: ";
    getline(cin, key);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);  //создаем сокет
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(PORT);

    int attempts = 0;
    bool connected = false;
    while(attempts < 5){
        if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR){
            cout << "Connection failured...restarting in 5 sec" << endl;
            this_thread::sleep_for(chrono::seconds(5));
            attempts++;
        }
        else {
            connected = true;
            break;
        }
    }

    if (!connected){
        cout << "Failured to connect after 5 attempts" << endl;
        return 1;
        closesocket(clientSocket);
        WSACleanup();
    }

    // запуск потока для получения сообщений
    thread(receiveMessages, clientSocket, key).detach();

    string message;
    while (true) {
        getline(cin, message);

        string encryptedMessage = shifruem(message, key);

        if (send(clientSocket, encryptedMessage.c_str(), encryptedMessage.size(), 0) == SOCKET_ERROR) {
            cout << "Send error." << endl;
            break;
        }

    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}