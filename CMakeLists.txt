cmake_minimum_required(VERSION 3.16)

project(rectangle_detector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV 4 REQUIRED)

add_executable(rectangle_detector main.cpp)
target_link_libraries(rectangle_detector PRIVATE ${OpenCV_LIBS})
target_include_directories(rectangle_detector PRIVATE ${OpenCV_INCLUDE_DIRS})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rectangle_detector PRIVATE -Wall -Wextra -Wpedantic)
endif()
