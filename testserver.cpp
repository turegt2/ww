#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <algorithm>
#pragma comment(lib, "ws2_32.lib")

#define PORT 5052
using namespace std;

vector<SOCKET> clients;
int i=1;

void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived=0;

    while (true) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            cout << "client disconnected " << endl;
            i-=1;
            break;
        }

        // Отправка сообщения всем подключенным клиентам, кроме того, кто его отправляет
        for (SOCKET client : clients) {
            if (client != clientSocket) {
                send(client, buffer, bytesReceived, 0);
                
            }
        }
    }

    closesocket(clientSocket);
    // удаляем клиента из списка
    clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);  //инициализируем winsock

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0); //создаем серверный сокет
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));  //привязываем сокет
    listen(serverSocket, SOMAXCONN); //сервер начинает принимать сообщения

    cout << "server is working . wait connection..." << endl;
    
    while (true) {
        
        SOCKET clientSocket = accept(serverSocket, NULL, NULL); //подтверждение подключения к сокету
        if (clientSocket == INVALID_SOCKET) {
            cout << "Client connection error." << endl;
            continue; 
        }

        clients.push_back(clientSocket);  //добавляем в вектор всех клиентов
        cout << "client " << i <<" has been connected." << endl;

        thread(handleClient, clientSocket).detach();  // обработка клиента в отдельном потоке

        if (i==3){
            cout << "A third-party client is connected, there is a danger of information interception " << endl;
            break;
        }
        i+=1;
    }

    // закрываем все сокеты
    for (SOCKET client : clients) {
        closesocket(client);
    }
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

