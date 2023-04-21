#include <iostream>
#include <string>
#include "../include/processing_server.hpp"

int main(int argc, char* argv[]) {
    // Default values
    std::string host = "127.0.0.1";
    int hostPort = 8080;
    std::string displayServerIP = "127.0.0.1";
    int displayServerPort = 8081;

    if (argc != 1 && argc != 3 && argc != 5) {
        std::cerr << "Usage: processing_server.exe [<display server address>"
                     " <display server port>] [<host server address> <host server port>]\n";
    } else if (argc == 3) {
        displayServerIP = argv[1];
        displayServerPort = std::stoi(argv[2]);
    } else if (argc == 5) {
        host = argv[3];
        hostPort = std::stoi(argv[4]);
    }

    DataProcessingServer server(host, hostPort, displayServerIP, displayServerPort);
    server.start();

    return 0;
}
