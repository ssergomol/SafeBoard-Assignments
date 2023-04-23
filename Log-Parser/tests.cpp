#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <string>
#include <filesystem>
#include "logs_parser.hpp"

using namespace testing;
using namespace std;

// Define a fixture class for testing
class LogParserTest : public Test {
public:
    void SetUp() override {
        // Create test directories and log files
        createDirectory("oneLog");
        createDirectory("logs1");
        createDirectory("logs1/subdir1");
        createDirectory("logs2");
        createLogFile("oneLog/process1.log", logContentSimple);
        createLogFile("logs1/process1.log", logContent1);
        createLogFile("logs1/subdir1/process1.log", logContent2);
        createLogFile("logs2/process2.log", logContent3);
        createDirectory("logs");
        createLogFile("logs/process.log", logContent4);
    }

    void TearDown() override {
        // Remove test directories and log files
        removeDirectory("logs1");
        removeDirectory("logs2");
    }

protected:
    // Define test log content
    const string logContentSimple = "[2023-04-06T09:32:01.041][Info][process1]simple-log";

    const string logContent1 = "[2023-04-06T09:32:01.041][Trace][process1] log message 1\n"
                               "[2023-04-06T09:33:19.765][Info][process1] something else\n"
                               "[2023-04-06T09:35:15.123][Debug][process1] jumping lazy fox\n"
                               "[2023-04-06T09:36:59.987][Warn][process1][part of the log message]Calculations completed\n"
                               "[2023-04-06T09:38:22.321][Error][process1][][][][ 4ds log message[]fs}{}\n"
                               "[2023-04-06T09:46:45.246][Info][process1] log message 6";

    const string logContent2 = "[2023-04-06T09:42:12.789][Trace][process1] #$@#252{}@#$2]34\n"
                               "[2023-04-06T09:44:16.980][Warn][process1] log message 3";

    const string logContent3 = "[2023-04-06T09:40:17.530][Debug][process2] log message 2\n"
                               "[2023-04-06T09:48:10.333][Error][process2] log message 4";

    const string logContent4 = "[2023-04-06T09:40:17.530][Debug][process1] log message 2\n"
                               "[2023-04-06T09:48:10.333][Error][process2] log message 4\n"
                               "[2023-04-06T09:48:10.333][Error][process2] log message 4\n"
                               "[2023-04-06T09:48:10.333][Warn][process1] log message 4\n";

    // Define helper functions
    void createDirectory(const string &path) {
        filesystem::create_directories(path);
    }

    void removeDirectory(const string &path) {
        filesystem::remove_all(path);
    }

    void createLogFile(const string &path, const string &content) {
        ofstream file(path);
        file << content;
        file.close();
    }

    void assertLogCountsOneLog(const unordered_map<string, unordered_map<string, int>> &logCounts) {
        // Check the log counts for process1
        ASSERT_THAT(logCounts.count("process1"), Eq(1));
        const auto &process1Counts = logCounts.at("process1");
        EXPECT_THAT(process1Counts.count("Info"), Eq(1));
        EXPECT_THAT(process1Counts.at("Info"), Eq(1));
    }

    void assertLogCounts1(const unordered_map<string, unordered_map<string, int>> &logCounts) {
        // Check the log counts for process1
        ASSERT_THAT(logCounts.count("process1"), Eq(1));
        const auto &process1Counts = logCounts.at("process1");
        EXPECT_THAT(process1Counts.count("Trace"), Eq(1));
        EXPECT_THAT(process1Counts.at("Trace"), Eq(2));
        EXPECT_THAT(process1Counts.count("Debug"), Eq(1));
        EXPECT_THAT(process1Counts.at("Debug"), Eq(1));
        EXPECT_THAT(process1Counts.count("Info"), Eq(1));
        EXPECT_THAT(process1Counts.at("Info"), Eq(2));
        EXPECT_THAT(process1Counts.count("Warn"), Eq(1));
        EXPECT_THAT(process1Counts.at("Warn"), Eq(2));
    }

    void assertLogCounts2(const unordered_map<string, unordered_map<string, int>> &logCounts) {
        // Check the log counts for process2
        ASSERT_THAT(logCounts.count("process2"), Eq(1));
        const auto &process2Counts = logCounts.at("process2");
        EXPECT_THAT(process2Counts.count("Debug"), Eq(1));
        EXPECT_THAT(process2Counts.at("Debug"), Eq(1));
        EXPECT_THAT(process2Counts.count("Error"), Eq(1));
        EXPECT_THAT(process2Counts.at("Error"), Eq(1));
    }

    void assertLogCountsBoth(const unordered_map<string, unordered_map<string, int>> &logCounts) {
        // Check the log counts for both processes
        assertLogCounts1(logCounts);
        assertLogCounts2(logCounts);
    }

    void assertLogCountsMixed(const unordered_map<string, unordered_map<string, int>> &logCounts) {
        // Check the log counts for mixed processes data

        ASSERT_THAT(logCounts.count("process1"), Eq(1));
        const auto &process1Counts = logCounts.at("process1");
        EXPECT_THAT(process1Counts.count("Debug"), Eq(1));
        EXPECT_THAT(process1Counts.at("Debug"), Eq(1));
        EXPECT_THAT(process1Counts.count("Warn"), Eq(1));
        EXPECT_THAT(process1Counts.at("Warn"), Eq(1));


        ASSERT_THAT(logCounts.count("process2"), Eq(1));
        const auto &process2Counts = logCounts.at("process2");
        EXPECT_THAT(process2Counts.count("Error"), Eq(1));
        EXPECT_THAT(process2Counts.at("Error"), Eq(2));
    }
};


TEST_F(LogParserTest, SimpleTest) {
    std::ifstream file("oneLog/process1.log");
    std::string log;
    std::getline(file, log);
    LogParser::LogInfo data = LogParser::parseLogLine(log);
    EXPECT_THAT(data.process, Eq("process1"));
    EXPECT_THAT(data.level, Eq("Info"));
}

TEST_F(LogParserTest, OneLog) {
    LogParser parser;
    parser.processLogDirectory("oneLog");
    assertLogCountsOneLog(parser.logCounts);
}

TEST_F(LogParserTest, Logs1) {
    LogParser parser;
    parser.processLogDirectory("logs1");
    assertLogCounts1(parser.logCounts);
}

TEST_F(LogParserTest, Logs2) {
    LogParser parser;
    parser.processLogDirectory("logs2");
    assertLogCounts2(parser.logCounts);
}

TEST_F(LogParserTest, Logs1_And_Logs2) {
    LogParser parser;
    parser.processLogDirectory("logs1");
    parser.processLogDirectory("logs2");
    assertLogCountsBoth(parser.logCounts);
}

TEST_F(LogParserTest, MixedLogs) {
    LogParser parser;
    parser.processLogDirectory("logs");
    assertLogCountsMixed(parser.logCounts);
}


