#include <iostream>
#include <string>
#include "display_server.hpp"

int main(int argc, char* argv[]) {
    // Default values
    std::string address = "127.0.0.1";
    int port =  8081;

    if (argc == 3) {
        address = argv[1];
        port = std::stoi(argv[2]);
    } else if (argc != 1) {
        std::cerr << "Usage: display_server.exe [<server address> <server port>]" << std::endl;
        return 1;
    }

    ResultDisplayServer server(address, port);
    server.start();
}

