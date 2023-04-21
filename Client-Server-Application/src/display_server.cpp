#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include "../include/display_server.hpp"
#pragma comment(lib, "ws2_32.lib")

ResultDisplayServer::ResultDisplayServer(std::string ipAddr, int port)
        : m_host(ipAddr), m_port(port) {

    WSADATA m_wsaData;
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &m_wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock.\n";
        throw std::runtime_error("Failed to initialize Winsock.");
    }

    // Create listening socket
    m_listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listeningSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket.\n";
        throw std::runtime_error("Failed to create socket.");
    }

    // Bind socket to address
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // inet_addr(m_host.c_str());
    addr.sin_port = htons(m_port);
    if (bind(m_listeningSocket, reinterpret_cast<SOCKADDR *>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind socket to address.\n";
        throw std::runtime_error("Failed to bind socket to address");
    }
}

void ResultDisplayServer::start() {
    std::cout << "Waiting for the processing server to connect..." << std::endl;

    // Listen for incoming connections
    if (listen(m_listeningSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Failed to listen for incoming connections: " << WSAGetLastError() << "\n";
        throw std::runtime_error("Failed to listen for incoming connections.");
    }

    // Wait for a connection
    sockaddr_in processingServer;
    int processingSize = sizeof(processingServer);

    m_processingServerSocket = accept(m_listeningSocket,
                                      reinterpret_cast<sockaddr*>(&processingServer), &processingSize);

    if (m_processingServerSocket == INVALID_SOCKET) {
        std::cerr << "Failed to accept the processing server connection: " << WSAGetLastError() << '\n';
        closesocket(m_listeningSocket);
        WSACleanup();
        return;
    }

    std::cout << "Processing server is connected\n";
    std::cout << "Start displaying messages:\n";


    // Create buffers for data receiving in chunks
    char tempBuffer[m_buffer_size];
    while(true) {

        std::string buffer;
        int bytesReceived = recv(m_processingServerSocket, reinterpret_cast<char *>(&tempBuffer),
                                 sizeof(tempBuffer), 0);
        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "Failed to receive data from client: " << WSAGetLastError() << '\n';
            break;
        } else if (bytesReceived == 0) {
            std::cout << "Processing server disconnected.\n";
            closesocket(m_processingServerSocket);
            break;

        } else {
            buffer.append(tempBuffer, bytesReceived);
            u_long nonBlockingMode = 1;

            if (ioctlsocket(m_processingServerSocket, FIONBIO, &nonBlockingMode) != NO_ERROR) {
                std::cout << "Failed to set non-blocking socket mode: " << WSAGetLastError() << '\n';
                break;
            }

            do {
                bytesReceived = recv(m_processingServerSocket, tempBuffer, sizeof(tempBuffer), 0);
                if (bytesReceived > 0) {
                    buffer.append(tempBuffer, bytesReceived);
                }

            } while (bytesReceived > 0);

            nonBlockingMode = 0;
            if (ioctlsocket(m_processingServerSocket, FIONBIO, &nonBlockingMode) != NO_ERROR) {
                std::cout << "Failed to set blocking socket mode: " << WSAGetLastError() << '\n';
                break;
            }

            std::cout << buffer << "\n";
        }
    }
}
