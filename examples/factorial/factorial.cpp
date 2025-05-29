#include <caskell.hpp>
#include <iostream>

using namespace caskell;

int factorial(int n) {
  return LazyStream(RangeGenerator<int>(1)).take(n).reduce(1, [](int a, int b) {
    return a * b;
  });
}

int main() {
  std::cout << factorial(10) << std::endl;
  return 0;
}