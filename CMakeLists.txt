cmake_minimum_required(VERSION 3.10)
project(caskell)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CASKELL_BUILD_TESTS "Build tests" OFF)
option(CASKELL_BUILD_EXAMPLES "Build examples" OFF)

add_library(caskell INTERFACE)

target_include_directories(caskell INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if (CASKELL_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if (CASKELL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

install(DIRECTORY include/ DESTINATION include)
install(TARGETS caskell
    EXPORT caskell-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
