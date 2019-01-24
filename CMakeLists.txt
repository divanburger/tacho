cmake_minimum_required(VERSION 3.10.2)
project(tacho)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2 -maes")

find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})

add_executable(tacho main.cpp string.cpp timeline.cpp memory.cpp files.cpp ui.cpp hash_table.cpp hash_table.h timeline_ui.cpp timeline_ui.h)

target_link_libraries(tacho ${SDL2_LIBRARIES} cairo)