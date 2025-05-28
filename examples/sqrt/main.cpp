#include <caskell.hpp>
#include <cmath>
#include <functional>
#include <iostream>

using namespace caskell;

// Pattern matching implementation for sqrt calculation
template <typename T> struct SqrtState {
  T x;
  T guess;
};

double mysqrt(double x) {
  SqrtState<double> initial{x, 1.0};

  std::function<double(const SqrtState<double> &)> iter =
      [&iter](const SqrtState<double> &state) -> double {
    return match<SqrtState<double>, double>(state)
        .with(guard<SqrtState<double>>([](const auto &s) {
                return std::abs(s.guess * s.guess - s.x) < 0.0001;
              }),
              [](const auto &s) { return s.guess; })
        .with(wildcard<SqrtState<double>>(), [&iter](const auto &s) {
          double new_guess = (s.guess + s.x / s.guess) / 2.0;
          return iter(SqrtState<double>{s.x, new_guess});
        });
  };

  return iter(initial);
}

int main() {
  double x = 2.0;
  double result = mysqrt(x);
  std::cout << "The square root of " << x << " is approximately " << result
            << std::endl;
  return 0;
}