cmake_minimum_required(VERSION 3.10.2)
project(tacho)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2 -maes")

find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})

add_executable(tacho main.cpp str.cpp timeline.cpp memory.cpp files.cpp ui.cpp hash_table.cpp timeline_ui.cpp array.cpp heap_ui.cpp heap.cpp json.cpp allocator.cpp)

target_link_libraries(tacho ${SDL2_LIBRARIES} cairo)


add_executable(output_label output_label.cpp)

target_link_libraries(output_label)


add_executable(request_viewer request_viewer.cpp str.cpp memory.cpp files.cpp ui.cpp hash_table.cpp array.cpp json.cpp allocator.cpp)

target_link_libraries(request_viewer ${SDL2_LIBRARIES} cairo)