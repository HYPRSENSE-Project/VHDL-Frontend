cmake_minimum_required(VERSION 3.20)
project(mini_vhdl_frontend LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(ExternalProject)
include(CTest)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Debug containers change the libstdc++ ABI, so every C++ target (including
# FetchContent dependencies) must use the same setting.
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_GLIBCXX_DEBUG")

add_subdirectory(src)
add_subdirectory(tests)
