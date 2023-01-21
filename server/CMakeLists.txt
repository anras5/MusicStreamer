cmake_minimum_required(VERSION 3.12)
project(sieci)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CMAKE_CXX_STANDARD 20)


add_executable(server src/server.cpp src/helpers.cpp)
