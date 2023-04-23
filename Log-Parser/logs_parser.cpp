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

// Define function to process all log files in a directory
void LogParser::processLogDirectory(const std::string &dirPath) {
    // Iterate over all files in the directory
    for (const auto &file: std::filesystem::recursive_directory_iterator(dirPath)) {
        // Process only .log files
        if (file.is_regular_file() && file.path().extension() == ".log") {
            // Process the file in a separate thread
            threads.emplace_back(&LogParser::processLogFile, this, file.path());
        }
    }

    wait();
    threads.clear();
}

// Define function to print the log counts as a table
void LogParser::printLogCounts() {
    // Collect the process names
    std::vector<std::string> processes;
    for (const auto &[process, counts]: logCounts) {
        processes.push_back(process);
    }

    // Sort the process names by total log count
    std::sort(processes.begin(), processes.end(), [&](const std::string &a, const std::string &b) {
        int aCount = 0;
        int bCount = 0;
        for (const auto &[level, count]: logCounts[a]) {
            aCount += count;
        }
        for (const auto &[level, count]: logCounts[b]) {
            bCount += count;
        }
        return aCount > bCount;
    });

    // Print the table header
    std::cout << std::left << std::setw(20) << "Process" << std::left << std::setw(20) << "Trace"
              << std::left << std::setw(20) << "Debug" << std::left << std::setw(20) << "Information" << std::left
              << std::setw(20) << "Warn" << std::left << std::setw(20) << "Error" << std::right << std::endl;
    // Print the log counts for each process
    for (const auto &process: processes) {
        std::cout << std::left << std::setw(20) << process;
        for (int i = 0; i < 5; ++i) {
//                LogLevel level = static_cast<LogLevel>(i);
            int count = logCounts[process][levels[i]];
            if (i != 4) {
                std::cout << std::left << std::setw(20) << count;
            } else {
                std::cout << std::left << std::setw(20) << count << std::right;
            }
        }
        std::cout << '\n';
    }

}

// Define function to process a single log file
void LogParser::processLogFile(const std::string &filePath) {
    // Open the file
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: could not open file " << filePath << std::endl;
        return;
    }

    // Read each line of the file
    std::string line;
    while (std::getline(file, line)) {
        // Parse the line
        LogInfo logInfo = parseLogLine(line);

        // Increment the log count for the process and level
        mtx.lock();
        ++logCounts[logInfo.process][logInfo.level];
        mtx.unlock();
    }

    // Close the file
    file.close();
}

void LogParser::wait() {
    for (auto &item: threads) {
        item.join();
    }
}

LogParser::LogInfo LogParser::parseLogLine(const std::string& line) {
    std::regex pattern(R"(\[(.*?)\]\[(.*?)\]\[(.*?)\])");
    std::smatch matches;
    std::string timestamp, log_level, process_name;
    if (std::regex_search(line, matches, pattern)) {
        timestamp = matches[1];
        log_level = matches[2];
        process_name = matches[3];
    }

    return {log_level, process_name};
}

std::vector<std::string> LogParser::levels{"Trace", "Debug", "Info", "Warn", "Error"};
