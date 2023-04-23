cmake_minimum_required(VERSION 3.23)
project(Logs_Parser)

set(CMAKE_CXX_STANDARD 20)

cmake_minimum_required(VERSION 3.23)
find_package(GTest REQUIRED)


add_executable(Logs_Parser tests.cpp logs_parser.cpp logs_parser.hpp)

# Link against GTest and GMock
target_link_libraries(Logs_Parser PRIVATE GTest::gmock_main)

