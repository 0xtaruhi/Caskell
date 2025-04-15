find_package(doctest)

add_executable(caskell_tests
    caskell_test.cpp
)
target_link_libraries(caskell_tests PRIVATE doctest::doctest)
target_link_libraries(caskell_tests PRIVATE caskell)