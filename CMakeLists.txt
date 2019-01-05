cmake_minimum_required(VERSION 3.13)
project(tachometer_reader)

set(CMAKE_CXX_STANDARD 14)

add_executable(tachometer_reader main.cpp string.cpp timeline.cpp timeline.h memory.cpp memory.h)