#include <iostream>
#include <string>
#include <WinSock2.h>
#include "../include/client.hpp"
#pragma comment(lib, "ws2_32.lib")

Client::Client(std::string address, int port) : m_address(address), m_port(port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock: " << WSAGetLastError() << '\n';
        throw std::runtime_error("Failed to initialize Winsock.");
    }

    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket: " << WSAGetLastError() << '\n';
        throw std::runtime_error("Failed to create socket.");
    }

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(m_address.c_str());
    serverAddr.sin_port = htons(m_port);

    if (connect(m_sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to server: " << WSAGetLastError() << '\n';
        throw std::runtime_error("Failed to connect to server.");
    }
}

Client::~Client() {
    closesocket(m_sock);
    WSACleanup();
}

void Client::send(const std::string& data) const {

    size_t netSize = htonl(data.size());
    if (::send(m_sock, reinterpret_cast<char*>(&netSize), sizeof(netSize), 0) < 0) {
        std::cerr << "Failed to send data: " << WSAGetLastError() << '\n';
        return;
    }

    if (::send(m_sock, data.c_str(), data.size(), 0) < 0) {
        std::cerr << "Failed to send data: " << WSAGetLastError() << '\n';
        return;
    }

    bool received{};
    if (::recv(m_sock, reinterpret_cast<char*>(&received), sizeof(received), 0) <= 0 || !received) {
        std::cerr << "Data wasn't transmitted entirely to the server.\n";
        return;
    }
    std::cout << "Data successfully transmitted to the server\n";
}
