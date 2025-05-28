#include <caskell.hpp>
#include <iostream>

using namespace caskell;

int factorial(int n) {
  return match(n) | (value(0) >> [](int) { return 1; })
         | (_ >> [](int n) { return n * factorial(n - 1); });
}

int main() {
  std::cout << factorial(5) << std::endl;
  return 0;
}