#ifndef DISPLAY_SERVER_DISPLAY_SERVER_HPP
#define DISPLAY_SERVER_DISPLAY_SERVER_HPP
#include <iostream>
#include <string>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

class ResultDisplayServer {
public:
    ResultDisplayServer(std::string ipAddr, int port);
    void start();

private:
    SOCKET m_processingServerSocket;
    SOCKET m_listeningSocket;
    std::string m_host;
    int m_port;
    static constexpr int m_buffer_size = 64;
};

#endif //DISPLAY_SERVER_DISPLAY_SERVER_HPP
