
#include <iostream>
#include <string>
#include "client.hpp"

int main(int argc, char* argv[]) {
    // Deafault values
    std::string address = "127.0.0.1";
    int port = 8080;

    if (argc != 1 && argc != 3) {
        std::cerr << "Usage: client.exe [<server address> <server port>]" << std::endl;
        return 1;
    }

    address = argv[1];
    port = std::stoi(argv[2]);

    Client client(address, port);

    std::string input;
    while (std::getline(std::cin, input)) {
        if (input.empty()) {
            continue;
        }

        client.send(input + "\n");
    }

    return 0;
}
