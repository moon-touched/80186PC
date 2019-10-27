cmake_minimum_required(VERSION 3.14)
project(80186PC)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

find_package(SDL2 REQUIRED)

add_subdirectory(80186PC)
add_subdirectory(bios)
