find_package(doctest)

add_executable(caskell_tests
    caskell_test.cpp
)
target_include_directories(caskell_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(caskell_tests PRIVATE doctest::doctest)