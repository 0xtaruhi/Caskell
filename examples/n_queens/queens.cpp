#include <caskell.hpp>
#include <iostream>

using namespace caskell;

// safe :: Int -> [Int] -> Int -> Bool
const auto safe = curry([](int x, const List<int> &qs, int y) {
  auto safe_rec = make_y_combinator([](auto self, int x, const List<int> &qs,
                                       int y) -> bool {
    if (qs.null())
      return true;

    auto q = qs.head();
    auto rest = qs.tail();

    return x != q &&
           std::abs(x - q) != std::abs(y - static_cast<int>(rest.length())) &&
           self(x, rest, y);
  });
  return safe_rec(x, qs, y);
});

// queens :: Int -> List [Int]
const auto queens = make_y_combinator([](auto self, int n) -> List<List<int>> {
  if (n == 0)
    return List<List<int>>({List<int>()});

  // [q:qs | qs <- queens (n-1), q <- [1..8], safe q qs (n-1)]
  return self(n - 1).from([n](const List<int> &qs) {
    return range(1, 8).from([n, qs](int q) {
      return safe(q)(qs)(n - 1) ? List<List<int>>({q | qs}) : List<List<int>>();
    });
  });
});

int main() {
  auto solutions = queens(8);
  std::cout << "Solutions: " << Show<List<List<int>>>::show(solutions) << "\n";
  std::cout << "Number of solutions: " << solutions.length() << std::endl;
  return 0;
}
