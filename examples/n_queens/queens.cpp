#include <caskell.hpp>
#include <iostream>

using namespace caskell;

// safe :: Int -> [Int] -> Int -> Bool
const auto safe = curry([](int x, const List<int> &qs, int y) {
  auto safe_rec = make_y_combinator([](auto self, int x, const List<int> &qs,
                                       int y) -> bool {
    return match(qs)
           | (value(List<int>()) >> [](const List<int> &) { return true; })
           | (_ >> [&self, x, y](const List<int> &qs) {
               return x != qs.head()
                      && std::abs(x - qs.head())
                             != std::abs(y - static_cast<int>(qs.length() - 1))
                      && self(x, qs.tail(), y);
             });
  });
  return safe_rec(x, qs, y);
});

// queens :: Int -> List [Int]
const auto queens = make_y_combinator([](auto self, int n) -> List<List<int>> {
  return match(n)
         | (value(0) >> [](int) { return List<List<int>>({List<int>()}); })
         | (_ >> [&self](int n) {
             return self(n - 1).from([n](const List<int> &qs) {
               return range(1, 8).from([n, qs](int q) {
                 return safe(q)(qs)(n - 1) ? List<List<int>>({q | qs})
                                           : List<List<int>>();
               });
             });
           });
});

int main() {
  auto solutions = queens(8);
  std::cout << "Solutions: " << std::endl; 
  std::cout << Show<List<List<int>>>::show(solutions) << "\n";
  std::cout << "Number of solutions: " << solutions.length() << std::endl;
  return 0;
}
