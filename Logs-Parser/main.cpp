#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <thread>
#include <string>
#include <regex>
#include <mutex>
#include <chrono>
#include "logs_parser.hpp"

int main(int argc, char* argv[]) {
    if (argc == 2) {
        LogParser parser;

        parser.processLogDirectory(argv[1]);
        parser.wait();
        parser.printLogCounts();
    } else {
        std::cerr << "Usage: log-parser <dir path>\n";
        return 1;
    }
}
