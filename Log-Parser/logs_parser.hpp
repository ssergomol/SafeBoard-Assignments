#ifndef LOGS_PARSER_LOGS_PARSER_HPP
#define LOGS_PARSER_LOGS_PARSER_HPP

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

class LogParser {
public:
    void processLogDirectory(const std::string& dirPath);
    void printLogCounts();
    void processLogFile(const std::string& filePath);
    void wait();

public:
    // Struct for log information
    struct LogInfo {
        std::string level;
        std::string process;
    };

    static LogParser::LogInfo parseLogLine(const std::string& line);

    std::mutex mtx;
    // key - process name, value - another map (key - log level, value - number of occurrences)
    std::unordered_map<std::string, std::unordered_map<std::string, int>> logCounts;
    std::vector<std::thread> threads;
    static std::vector<std::string> levels;
};


#endif //LOGS_PARSER_LOGS_PARSER_HPP
