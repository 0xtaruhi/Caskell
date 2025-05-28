#include <caskell.hpp>
#include <cmath>
#include <iostream>

using namespace caskell;

double mysqrt(double x) {
  double guess = 1.0;

  const auto iter =
      make_y_combinator([](auto self, double x, double guess) -> double {
        const auto goodEnough = [](double x, double guess) {
          return std::abs(guess * guess - x) < 0.0001;
        };
        const auto improve = [](double x, double guess) {
          return (guess + x / guess) / 2.0;
        };

        return match(x, guess)
            .with(guard(goodEnough), [](double, double guess) { return guess; })
            .with(wildcard(), [&self, improve](double x, double guess) {
              return self(x, improve(x, guess));
            });
      });

  return iter(x, guess);
}

int main() {
  double x = 2.0;
  double result = mysqrt(x);
  std::cout << "The square root of " << x << " is approximately " << result
            << std::endl;
  return 0;
}