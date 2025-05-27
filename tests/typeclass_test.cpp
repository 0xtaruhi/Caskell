#include "typeclass.hpp"
#include <doctest/doctest.h>

TEST_CASE("Functor") {
  SUBCASE("Maybe Functor") {
    auto addOne = [](int x) { return x + 1; };
    auto maybeInt = caskell::Maybe<int>(5);
    auto result = caskell::fmap<caskell::Maybe>(
        maybeInt, std::function<int(int)>(addOne));
    CHECK(result.isJust());
    CHECK(*result == 6);

    auto nothing = caskell::Maybe<int>();
    auto result2 =
        caskell::fmap<caskell::Maybe>(nothing, std::function<int(int)>(addOne));
    CHECK(result2.isNothing());
  }
}

TEST_CASE("Applicative") {
  SUBCASE("Maybe Applicative") {
    auto add = [](int x) { return [x](int y) { return x + y; }; };
    auto maybeAdd = caskell::Maybe<std::function<int(int)>>(add(5));
    auto maybeInt = caskell::Maybe<int>(3);

    auto result = caskell::ap<caskell::Maybe>(maybeAdd, maybeInt);
    CHECK(result.isJust());
    CHECK(*result == 8);

    auto nothing = caskell::Maybe<int>();
    auto result2 = caskell::ap<caskell::Maybe>(maybeAdd, nothing);
    CHECK(result2.isNothing());
  }
}

TEST_CASE("Monad") {
  SUBCASE("Maybe Monad") {
    auto safeDiv = [](int x) {
      return [x](int y) {
        if (y == 0)
          return caskell::Maybe<int>();
        return caskell::Maybe<int>(x / y);
      };
    };

    auto maybeInt = caskell::Maybe<int>(10);
    auto result = caskell::bind<caskell::Maybe>(
        maybeInt, std::function<caskell::Maybe<int>(int)>(
                      [safeDiv](int x) { return safeDiv(x)(2); }));
    CHECK(result.isJust());
    CHECK(*result == 5);

    auto nothing = caskell::Maybe<int>();
    auto result2 = caskell::bind<caskell::Maybe>(
        nothing, std::function<caskell::Maybe<int>(int)>(
                     [safeDiv](int x) { return safeDiv(x)(2); }));
    CHECK(result2.isNothing());
  }

  SUBCASE("Monad Laws") {
    // Left identity: return a >>= f ≡ f a
    auto f = [](int x) { return caskell::Maybe<int>(x + 1); };
    auto a = 5;
    auto left1 = caskell::bind<caskell::Maybe>(
        caskell::return_<caskell::Maybe>(a),
        std::function<caskell::Maybe<int>(int)>(f));
    auto left2 = f(a);
    CHECK(*left1 == *left2);

    // Right identity: m >>= return ≡ m
    auto m = caskell::Maybe<int>(5);
    auto returnFunc = [](int x) { return caskell::return_<caskell::Maybe>(x); };
    auto right1 = caskell::bind<caskell::Maybe>(
        m, std::function<caskell::Maybe<int>(int)>(returnFunc));
    CHECK(*right1 == *m);

    // Associativity: (m >>= f) >>= g ≡ m >>= (\x -> f x >>= g)
    auto g = [](int x) { return caskell::Maybe<int>(x * 2); };
    auto fg = [f, g](int x) {
      return caskell::bind<caskell::Maybe>(
          f(x), std::function<caskell::Maybe<int>(int)>(g));
    };
    auto assoc1 = caskell::bind<caskell::Maybe>(
        caskell::bind<caskell::Maybe>(
            m, std::function<caskell::Maybe<int>(int)>(f)),
        std::function<caskell::Maybe<int>(int)>(g));
    auto assoc2 = caskell::bind<caskell::Maybe>(
        m, std::function<caskell::Maybe<int>(int)>(fg));
    CHECK(*assoc1 == *assoc2);
  }
}