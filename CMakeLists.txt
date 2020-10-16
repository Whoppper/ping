cmake_minimum_required(VERSION 3.17)
project(ping)

set(CMAKE_CXX_STANDARD 14)

add_executable(ping src/main.cpp inc/main.h)