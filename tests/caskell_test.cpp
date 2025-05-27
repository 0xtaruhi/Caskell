// #include "maybe.hpp"
#include "variant.hpp"
#include <string>
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
  SUBCASE("Lazy Stream with RangeGenerator") {
    auto lazyResult = caskell::LazyStream(caskell::RangeGenerator<int>(1))
                          .map([](int x) { return x * x; })
                          .filter([](int x) { return x % 3 == 0; })
                          .take(5)
                          .reduce(0, [](int a, int b) { return a + b; });
    CHECK(lazyResult ==
          9 + 36 + 81 + 144 + 225); // 0 + 3^2 + 6^2 + 9^2 + 12^2 + 15^2
  }

  SUBCASE("Lazy Stream with ContainerGenerator") {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto lazyResult = caskell::LazyStream(caskell::ContainerGenerator(vec))
                          .map([](int x) { return x * x; })
                          .collect<std::vector<int>>(); // Collect to vector
    CHECK(lazyResult.size() == 5);
    CHECK(lazyResult[0] == 1);
    CHECK(lazyResult[1] == 4);
    CHECK(lazyResult[2] == 9);
    CHECK(lazyResult[3] == 16);
    CHECK(lazyResult[4] == 25);
  }
}

TEST_CASE("Maybe") {
  auto safe_div = [](int a, int b) {
    if (b == 0)
      return caskell::Maybe<int>();
    return caskell::Maybe<int>(a / b);
  };

  auto add_5_if_even = [](int x) {
    if (x % 2 == 0)
      return caskell::Maybe<int>(x + 5);
    return caskell::Maybe<int>();
  };

  auto result =
      safe_div(100, 5) | add_5_if_even | caskell::curry(safe_div)(100);
  CHECK(result.value_or(-1) == 4);
}

TEST_CASE("Variant") {
  caskell::Variant<int, std::string> v1(42);
  auto f = [](const auto &v) {
    v.match([](int i) { std::cout << "int: " << i << std::endl; },
            [](const std::string &s) {
              std::cout << "string: " << s << std::endl;
            });
  };
  f(v1);
  v1 = std::string("Hello, Variant!");
  f(v1);
  v1 = 100;
  f(v1);
}
