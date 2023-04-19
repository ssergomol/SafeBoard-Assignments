#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <winsock2.h> // Windows sockets library

class DataProcessingServer {
public:
    DataProcessingServer(const std::string &host, int port, const std::string &resultDisplayHost, int resultDisplayPort)
            : m_host(host), m_port(port), m_resultDisplayHost(resultDisplayHost),
              m_resultDisplayPort(resultDisplayPort) {
        // Initialize Winsock
        if (WSAStartup(MAKEWORD(2, 2), &m_wsaData) != 0) {
            throw std::runtime_error("Failed to initialize Winsock.");
        }

        // Create socket
        m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_listenSocket == INVALID_SOCKET) {
            throw std::runtime_error("Failed to create socket.");
        }

        // Bind socket to address
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(m_host.c_str());
        addr.sin_port = htons(m_port);
        if (bind(m_listenSocket, (SOCKADDR * ) & addr, sizeof(addr)) == SOCKET_ERROR) {
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

        // Accept incoming connections
        while (true) {
            SOCKET clientSocket = accept(m_listenSocket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) {
                throw std::runtime_error("Failed to accept incoming connection.");
            }
            std::cout << "Client connected.\n";

            // Process client data
            processClientData(clientSocket);
        }
    }

private:
    std::string m_host;
    int m_port;
    std::string m_resultDisplayHost;
    int m_resultDisplayPort;
    WSADATA m_wsaData;
    SOCKET m_listenSocket;

    void processClientData(SOCKET clientSocket) {
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

        // Convert data to string of words separated by spaces
        std::vector<std::string> words = getWordsFromString(dataReceived);

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

        return 0;
    }

