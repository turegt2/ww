#pragma once

#include <winsock2.h>
#include <string>
#include <thread>

// Шифрование и дешифрование
std::string encryptMessage(const std::string& input, const std::string& key);
std::string decryptMessage(const std::string& input, const std::string& key);

// Обработка входящих сообщений
void receiveMessages(SOCKET socket, const std::string& key);

// Подключение к серверу
bool connectToServer(SOCKET& clientSocket, sockaddr_in& serverAddr);