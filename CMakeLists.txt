cmake_minimum_required(VERSION 3.10.2)
project(tacho)

set(CMAKE_CXX_STANDARD 14)

find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})

add_executable(tacho main.cpp string.cpp timeline.cpp memory.cpp files.cpp ui.cpp)

target_link_libraries(tacho ${SDL2_LIBRARIES} cairo)