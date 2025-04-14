#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "caskell.hpp"

TEST_CASE("Curry") {
  auto add3 = [](int a, int b, int c) { return a + b + c; };
  auto curried_add = caskell::curry(add3);

  CHECK(curried_add(1)(2)(3) == 6);
  CHECK(curried_add(1, 2)(3) == 6);
  CHECK(curried_add(1)(2, 3) == 6);
  CHECK(curried_add(1, 2, 3) == 6);

  auto add1 = caskell::curry(add3, 8);
  auto add12 = add1(2);
  CHECK(add12(3) == 13);
}

TEST_CASE("Lazy Stream") {
  auto lazyResult = caskell::LazyStream(caskell::RangeGenerator<int>(1))
                        .map([](int x) { return x * x; })
                        .filter([](int x) { return x % 3 == 0; })
                        .take(5)
                        .reduce(0, [](int a, int b) { return a + b; });
  CHECK(lazyResult ==
        9 + 36 + 81 + 144 + 225); // 0 + 3^2 + 6^2 + 9^2 + 12^2 + 15^2

  auto isPrime = [](int n) {
    if (n < 2)
      return false;
    for (int i = 2; i * i <= n; ++i) {
      if (n % i == 0)
        return false;
    }
    return true;
  };

  auto primeResult = caskell::LazyStream(caskell::RangeGenerator<int>(1))
                         .filter(isPrime)
                         .take(50000);
  for (auto i : primeResult) {
    std::cout << i << " ";
  }
}
