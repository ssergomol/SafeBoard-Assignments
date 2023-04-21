#ifndef CLIENT_SERVER_APPLICATION_PROCESSING_SERVER_HPP
#define CLIENT_SERVER_APPLICATION_PROCESSING_SERVER_HPP

#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <set>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")


class DataProcessingServer {
public:
    DataProcessingServer(std::string host, int port, std::string resultDisplayHost, int resultDisplayPort);
    ~DataProcessingServer();
    void start();

private:
    std::string m_host;
    int m_port;
    std::string m_resultDisplayHost;
    int m_resultDisplayPort;
    WSADATA m_wsaData;
    SOCKET m_listenSocket;
    SOCKET m_displayServerSocket;
    static constexpr int m_buffer_size = 64;

    struct ClientData{
        size_t messageSize{};
        std::string buffer;
    };

    static std::string removeDuplicates(const std::string& text);
    void connectDisplayServer();
};

#endif //CLIENT_SERVER_APPLICATION_PROCESSING_SERVER_HPP
