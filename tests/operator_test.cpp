#include "typeclass.hpp"
#include <doctest/doctest.h>

using namespace caskell::operators;

TEST_CASE("Operator Overloading") {
  SUBCASE("Functor Operator (>>)") {
    auto addOne = [](int x) { return x + 1; };
    auto maybeInt = caskell::Maybe<int>(5);
    auto result = addOne >> maybeInt;
    CHECK(result.isJust());
    CHECK(*result == 6);
  }

  SUBCASE("Applicative Operator (*)") {
    auto add = [](int x) { return [x](int y) { return x + y; }; };
    auto maybeAdd = caskell::Maybe(add(5));
    auto maybeInt = caskell::Maybe<int>(3);

    auto result = maybeAdd * maybeInt;
    CHECK(result.isJust());
    CHECK(*result == 8);
  }

  SUBCASE("Monad Operator (>>)") {
    auto safeDiv = [](int x) -> caskell::Maybe<int> {
      if (x == 0)
        return caskell::Maybe<int>();
      return caskell::Maybe<int>(10 / x);
    };

    auto maybeInt = caskell::Maybe<int>(2);
    auto result = maybeInt >>= safeDiv;
    CHECK(result.isJust());
    CHECK(*result == 5);

    auto nothing = caskell::Maybe<int>();
    auto result2 = nothing >>= safeDiv;
    CHECK(result2.isNothing());
  }

  SUBCASE("Then Operator (>)") {
    auto m1 = caskell::Maybe<int>(5);
    auto m2 = caskell::Maybe<int>(10);
    auto result = m1 > m2;
    CHECK(result.isJust());
    CHECK(*result == 10);
  }

  SUBCASE("Pipeline Operator (|)") {
    auto addOne = [](int x) { return x + 1; };
    auto result = 5 | addOne;
    CHECK(result == 6);
  }

  SUBCASE("Chained Operators") {
    auto addOne = [](int x) { return x + 1; };
    auto maybeInt = caskell::Maybe<int>(4);
    auto safeDiv = [](int x) -> caskell::Maybe<int> {
      if (x == 0)
        return caskell::Maybe<int>();
      return caskell::Maybe<int>(10 / x);
    };

    // (4 + 1) / 2
    auto result = maybeInt >> addOne >>= safeDiv;
    CHECK(result.isJust());
    CHECK(*result == 2);

    // 使用 pipeline 风格
    auto result2 = maybeInt | addOne | safeDiv;
    CHECK(result2.isJust());
    CHECK(*result2 == 2);
  }
}