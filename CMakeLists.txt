cmake_minimum_required(VERSION 3.28)
project(caskell)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(caskell src/caskell.cpp)

target_include_directories(caskell PRIVATE include)
