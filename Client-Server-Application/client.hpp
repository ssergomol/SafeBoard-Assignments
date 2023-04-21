#ifndef CLIENT_HPP
#define CLIENT_HPP
#include <iostream>
#include <string>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

class Client {
public:
    Client(std::string address, int port);
    void send(const std::string& data) const;
    ~Client();

private:
    SOCKET m_sock;
    std::string m_address;
    int m_port;
};

#endif //CLIENT_HPP
