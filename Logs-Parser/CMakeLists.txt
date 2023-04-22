cmake_minimum_required(VERSION 3.23)
project(Logs_Parser)

set(CMAKE_CXX_STANDARD 20)

add_executable(Logs_Parser main.cpp logs_parser.cpp logs_parser.hpp)
