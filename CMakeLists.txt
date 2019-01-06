cmake_minimum_required(VERSION 3.13)
project(tachometer_reader)

set(CMAKE_CXX_STANDARD 14)

find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})

add_executable(tachometer_reader main.cpp string.cpp timeline.cpp timeline.h memory.cpp memory.h)

target_link_libraries(tachometer_reader ${SDL2_LIBRARIES} cairo)