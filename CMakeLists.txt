cmake_minimum_required(VERSION 3.16)
project(f1_exchange LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(f1_exchange
    src/main.cpp
    src/Engine.cpp
    src/Replay.cpp
    src/Metrics.cpp
)

target_include_directories(f1_exchange PRIVATE src)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(f1_exchange PRIVATE -Wall -Wextra -Wpedantic -O2)
endif()
