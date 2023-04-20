#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <cstring>
#include <stdexcept>
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
            throw std::runtime_error("Failed to initialize Winsock.");
        }

        // Create listening socket
        m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_listenSocket == INVALID_SOCKET) {
            throw std::runtime_error("Failed to create socket.");
        }

        // Bind socket to address
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(m_host.c_str());
        addr.sin_port = htons(m_port);
        if (bind(m_listenSocket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to bind socket to address.");
        }
    }

    ~DataProcessingServer() {
        closesocket(m_listenSocket);
        WSACleanup();
    }

    void start() {
        std::cout << "Waiting for client connections...\n";

        // Listen for incoming connections
        if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to listen for incoming connections.");
        }

        // Create an event object for the listening socket
        HANDLE listenEvent = WSACreateEvent();
        if (listenEvent == WSA_INVALID_EVENT) {
            throw std::runtime_error("Failed to create event object.");
        }

        // Associate the event object with the listening socket
        if (WSAEventSelect(m_listenSocket, listenEvent, FD_ACCEPT) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to associate event object with the listening socket");
        }

        // Create a buffer for receiving data from clients
        char buffer[m_buffer_size];

        // Create a vector to store clients sockets
        std::vector<SOCKET> clientSockets;

        // Create a vector to hold event objects
        std::vector<WSAEVENT> events;

        while (true) {

            // resize the events vector to hold the current number of sockets
            events.resize(clientSockets.size() + 1);
            events[0] = listenEvent;

            // Add client sockets to the array of event objects
            for (size_t i = 0; i < clientSockets.size(); ++i) {
                events[i + 1] = WSACreateEvent();
                if (events[i + 1] == WSA_INVALID_EVENT) {
                    throw std::runtime_error("Failed to create event object.");
                }
                if (WSAEventSelect(clientSockets[i], events[i + 1], FD_READ | FD_CLOSE) == SOCKET_ERROR) {
                    throw std::runtime_error("Failed to associate event object with client socket.");
                }
            }

            // Wait for events to occur
            DWORD eventCount = WSAWaitForMultipleEvents(clientSockets.size() + 1, events.data(), FALSE, WSA_INFINITE, FALSE);
            if (eventCount == WSA_WAIT_FAILED) {
                throw std::runtime_error("Wait for events failed.");
            }

            // Process events
            if (eventCount == WSA_WAIT_FAILED) {
                throw std::runtime_error("WSAWaitForMultipleEvents failed.");

            } else if (eventCount == WSA_WAIT_EVENT_0) {
                // Accept a new connection
                sockaddr_in clientAddress;
                int clientAddressLength = sizeof(clientAddress);
                SOCKET clientSocket = accept(m_listenSocket, reinterpret_cast<sockaddr *>(&clientAddress),
                                             &clientAddressLength);
                if (clientSocket == INVALID_SOCKET) {
                    throw std::runtime_error("Failed to accept connection.");
                }
                clientSockets.push_back(clientSocket);
                std::cout << "Client is connected.\n";
            } else {
                int clientSocket = clientSockets[WSA_WAIT_EVENT_0 - 1];
                int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
                if (bytesReceived == SOCKET_ERROR) {
                    throw std::runtime_error("Failed to receive data from client.");
                } else if (bytesReceived == 0) {
                    std::cout << "Client disconnected.\n";
                    closesocket(clientSocket);
                    WSACloseEvent(events[WSA_WAIT_EVENT_0]);
                    clientSockets.erase(clientSockets.begin() + WSA_WAIT_EVENT_0 - 1);
                } else {
                    // Send data to the display server
                    std::cout << "Received data from client: " << buffer << "\n";
                }
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
    static constexpr int m_buffer_size = 1024;
    static constexpr int m_max_events = 64;

  /*  void processClientData(SOCKET clientSocket) {
        // Receive data from client
        char buffer[1024];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived == SOCKET_ERROR) {
            throw std::runtime_error("Failed to receive data from client.");
        }
        std::string dataReceived(buffer, bytesReceived);
        std::cout << "Data received from client: " << dataReceived << std::endl;

        // Verify data correctness
        if (!isDataCorrect(dataReceived)) {
            throw std::runtime_error("Invalid data received from client.");
        }

        // Send confirmation to client
        std::string confirmationMessage = "Data received in full.";
        int bytesSent = send(clientSocket, confirmationMessage.c_str(), confirmationMessage.size() + 1, 0);
        if (bytesSent == SOCKET_ERROR) {
            throw std::runtime_error("Failed to send confirmation to client.");
        }

        // Convert the received data into a string of words separated by spaces.
        std::vector<std::string> words = getWordsFromString(dataReceived);

// Find and delete duplicate words in this line.
        std::vector<std::string> uniqueWords;
        for (const std::string &word: words) {
            if (std::find(uniqueWords.begin(), uniqueWords.end(), word) == uniqueWords.end()) {
                uniqueWords.push_back(word);
            }
        }

// Convert the vector of unique words back into a string.
        std::string result = "";
        for (const std::string &word: uniqueWords) {
            result += word + " ";
        }

// Send the resulting result to the result display server.
        if (sendData(resultDisplaySocket, result) == -1) {
            std::cerr << "Error sending data to the result display server." << std::endl;
            closesocket(resultDisplaySocket);
            WSACleanup();
            return 1;
        }

// Close the connection to the result display server.
        closesocket(resultDisplaySocket);

// Close the connection to the client application.
        closesocket(clientSocket);

// Cleanup Winsock.
        WSACleanup();
    }*/
};

int main() {
    DataProcessingServer server("127.0.0.1", 8080, "127.0.0.1", 8081);
    server.start();

    return 0;
}

