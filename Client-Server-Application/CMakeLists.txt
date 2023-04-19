cmake_minimum_required(VERSION 3.23)
project(Client_Server_Application)

set(CMAKE_CXX_STANDARD 20)

add_executable(Client_Server_Application app.cpp display_server.cpp processing_server.cpp)
