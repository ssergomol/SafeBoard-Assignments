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
    DataProcessingServer(std::string host, int port, std::string resultDisplayHost, int resultDisplayPort)
            : m_host(std::move(host)), m_port(port), m_resultDisplayHost(std::move(resultDisplayHost)),
              m_resultDisplayPort(resultDisplayPort) {

        // Initialize Winsock
        if (WSAStartup(MAKEWORD(2, 2), &m_wsaData) != 0) {
            std::cerr << "Failed to initialize Winsock.\n";
        }

        // Create listening socket
        m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_listenSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket.\n";
        }

        // Bind socket to address
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY; // inet_addr(m_host.c_str());
        addr.sin_port = htons(m_port);
        if (bind(m_listenSocket, reinterpret_cast<SOCKADDR *>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "Failed to bind socket to address.\n";
        }

        // Connect to display server
        connectDisplayServer();
    }

    ~DataProcessingServer() {
        closesocket(m_listenSocket);
        WSACleanup();
    }

    void start() {
        std::cout << "Waiting for client connections...\n";

        // Listen for incoming connections
        if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Failed to listen for incoming connections: " << WSAGetLastError() << "\n";
            throw std::runtime_error("");
        }

        // Create an event object for the listening socket
        WSAEVENT listenEvent = WSACreateEvent();
        if (listenEvent == WSA_INVALID_EVENT) {
            std::cerr << "Failed to create event object: " << WSAGetLastError() << '\n';
            throw std::runtime_error("");
        }

        // Associate the event object with the listening socket
        if (WSAEventSelect(m_listenSocket, listenEvent, FD_ACCEPT) == SOCKET_ERROR) {
            std::cerr << "Failed to associate event object with the listening socket: " << WSAGetLastError() << '\n';
            throw std::runtime_error("");
        }


        // Create a buffer for receiving data from client in chunks
        char tempBuffer[m_buffer_size];

        // Create a vector to store clients sockets
        std::vector<SOCKET> clientSockets;
        clientSockets.push_back(m_listenSocket);

        // Create a vector to hold event objects
        std::vector<WSAEVENT> events;

        while (true) {

            // resize the events vector to hold the current number of sockets
            events.resize(clientSockets.size());
            events[0] = listenEvent;

            std::cout << "Number of sockets: " << clientSockets.size() << "\n";

            // Add client sockets to the array of event objects
            for (size_t i = 1; i < clientSockets.size(); ++i) {
                events[i] = WSACreateEvent();
                if (events[i] == WSA_INVALID_EVENT) {
                    throw std::runtime_error("Failed to create event object.");
                }
                if (WSAEventSelect(clientSockets[i], events[i], FD_READ | FD_CLOSE) == SOCKET_ERROR) {
                    throw std::runtime_error("Failed to associate event object with client socket.");
                }
            }


            // Wait for events to occur
            std::cout << "Start waiting for events\n";
            DWORD eventCount = WSAWaitForMultipleEvents(clientSockets.size(), events.data(),
                    FALSE, WSA_INFINITE, FALSE);
            std::cout << "End waiting for events\n";
            int eventIndex = eventCount - WSA_WAIT_EVENT_0;

            WSANETWORKEVENTS networkEvents;
            WSAEnumNetworkEvents(clientSockets[eventIndex], events[eventIndex], &networkEvents);

            // Process events
            if (eventCount == WSA_WAIT_FAILED) {
                throw std::runtime_error("WSAWaitForMultipleEvents failed.");

            } else if (eventIndex == 0 && networkEvents.lNetworkEvents & FD_ACCEPT) {

                // Accept a new connection
                sockaddr_in clientAddress;
                int clientAddressLength = sizeof(clientAddress);
                SOCKET clientSocket = accept(m_listenSocket, reinterpret_cast<sockaddr *>(&clientAddress),
                                             &clientAddressLength);
                if (clientSocket == INVALID_SOCKET) {
                    throw std::runtime_error("Failed to accept connection.");
                }
                clientSockets.push_back(clientSocket);
                std::cout << "Client is connected: " << "*:" << clientAddress.sin_port << '\n';

                int keepAlive = 1;
                int keepAliveInterval = 5;
                int keepAliveTime = 10;

                // Set keep alive option
                if (setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE,
                        reinterpret_cast<char*>(&keepAlive),
                        sizeof(keepAlive)) != 0) {
                    std::cerr << "Failed to set keep-alive option: " << WSAGetLastError() << "\n";
                    throw std::runtime_error("");
                }

                if (setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPIDLE,
                        reinterpret_cast<char*>(&keepAliveTime),
                        sizeof(keepAliveTime)) != 0) {
                    std::cerr << "Failed to set keep-alive time: " << WSAGetLastError() << "\n";
                    return;
                }

                if (setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPINTVL,
                        reinterpret_cast<char*>(&keepAliveInterval),
                        sizeof(keepAliveInterval)) != 0) {
                    std::cerr << "Failed to set keep-alive interval time: " << WSAGetLastError() << "\n";
                    return;
                }

            } else if (networkEvents.lNetworkEvents & FD_READ) {
                int clientSocket = clientSockets[eventIndex];
                size_t transmittedSize{};

                // Read data from client in chunks
                int bytesReceived = recv(clientSocket, reinterpret_cast<char*>(&transmittedSize),
                        sizeof(transmittedSize), 0);
                if (bytesReceived == SOCKET_ERROR) {
                    std::cerr << "Failed to receive data from client: " << WSAGetLastError() << '\n';
                    throw std::runtime_error("");
                } else if (bytesReceived == 0) {
                    std::cout << "Client disconnected.\n";
                    closesocket(clientSocket);
                    WSACloseEvent(events[eventIndex]);
                    clientSockets.erase(clientSockets.begin() + eventIndex);
                } else {
                    struct ClientData receivedData;
                    size_t totalBytesReceived = 0;
                    receivedData.messageSize = ntohl(transmittedSize);

                    do {
                        bytesReceived = recv(clientSocket, tempBuffer, sizeof(tempBuffer), 0);
                        if (bytesReceived > 0) {
                            receivedData.buffer.append(tempBuffer, bytesReceived);
                            totalBytesReceived += bytesReceived;
                        }

                    } while (bytesReceived > 0);

                    bool successfullyTransmitted = false;
                    if (totalBytesReceived == receivedData.messageSize) {
                        successfullyTransmitted = true;
                    }

                    if (send(clientSocket, reinterpret_cast<char*>(&successfullyTransmitted),
                            sizeof(successfullyTransmitted), 0) < 0) {
                        std::cerr << "Failed to send success message to client: " << WSAGetLastError() << "\n";
                    }

                    // Process data
                    std::string processedData = removeDuplicates(receivedData.buffer);

                    // TODO: Send data to the display server
                    std::cout << "Received data from client: " << receivedData.buffer << "\n";
                }

            } else if (networkEvents.lNetworkEvents & FD_CLOSE) {
                // Other socket end is closed
                int clientSocket = clientSockets[eventIndex];
                closesocket(clientSocket);
                WSACloseEvent(events[eventIndex]);
                clientSockets.erase(clientSockets.begin() + eventIndex);

                std::cout << "Client disconnected\n";
            }
        }
    }


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

    static std::string removeDuplicates(const std::string& text) {
            std::string input = "this is a test string string to test this";
            std::string output;
            std::istringstream iss(text);
            std::set<std::string> words;

            // tokenize the input text
            for (std::string word; iss >> word; ) {
                if (words.find(word) == words.end()) {
                    output += word + " ";
                    words.insert(word);
                }
            }

            // remove  trailing space
            output.pop_back();
            return output;
    }

    void connectDisplayServer() {
        m_displayServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_displayServerSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create display server socket: " << WSAGetLastError() << '\n';
            return;
        }

        SOCKADDR_IN serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(m_resultDisplayHost.c_str());
        serverAddr.sin_port = htons(m_resultDisplayPort);

        if (connect(m_displayServerSocket, reinterpret_cast<sockaddr*>(&serverAddr),
                sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Failed to connect to display server: " << WSAGetLastError() << '\n';
            return;
        }
    }
};

int main() {
    DataProcessingServer server("127.0.0.1", 8080, "127.0.0.1", 8081);
    server.start();

    return 0;
}
