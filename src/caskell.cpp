#include "caskell.hpp"
#include <iostream>
#include <utility>
#include <vector>

int add3(int a, int b, int c) { return a + b + c; }

caskell::Maybe<int> safe_div(int a, int b) {
  if (b == 0)
    return caskell::Maybe<int>();
  return caskell::Maybe<int>(a / b);
}

caskell::Maybe<int> add_5_if_even(int x) {
  if (x % 2 == 0)
    return caskell::Maybe<int>(x + 5);
  return caskell::Maybe<int>();
}

class Foo {
public:
  Foo(int x) : x_(x) {}

  int add(int y) { return x_ + y; }

private:
  int x_;
};

int main() {
  auto curried_add = caskell::curry(add3);

  std::cout << curried_add(1)(2)(3) << std::endl; // 输出 6
  std::cout << curried_add(1, 2)(3) << std::endl; // 输出 6
  std::cout << curried_add(1)(2, 3) << std::endl; // 输出 6
  std::cout << curried_add(1, 2, 3) << std::endl; // 输出 6

  auto add1 = caskell::curry(add3, 8);
  auto add12 = add1(2);
  std::cout << add12(3) << std::endl; // 输出 6

  auto curried_foo_add = caskell::curry(&Foo::add);
  Foo foo{1};
  auto foo_add1 = curried_foo_add(foo);
  auto foo_add2 = foo_add1(100);
  std::cout << foo_add2 << std::endl;

  auto vec = std::vector<int>{1, 2, 3, 4, 5};
  auto stream = caskell::stream(std::move(vec));
  auto result = stream.filter([](int x) { return x % 2 == 0; })
                    .map([](int x) { return x * 2; })
                    .map([](int x) { return x * x; })
                    .forEach([](int x) { std::cout << x << " "; })
                    .reduce([](int a, int b) { return a + b; }, 0);
  std::cout << "\nResult: " << result << std::endl; // 输出 80

  auto lazyResult = caskell::LazyStream<int>::fromRange(100)
                        .map([](int x) { return x * x; })
                        .filter([](int x) { return x % 3 == 0; })
                        .map([](int x) { return std::make_pair(x, x + 1); })
                        .take(5);

  for (auto item : lazyResult) {
    std::cout << item.first << " " << item.second << "   ";
  }
  std::cout << std::endl;

  caskell::Maybe<int> m1(10);
  auto r1 = m1 >>= [](int x) { return safe_div(x, 2); };
  std::cout << "Result of safe_div: " << r1.value_or(-1) << std::endl; // 输出 5
  auto r2 = m1.and_then([](int x) { return safe_div(x, 0); });
  std::cout << "Result of safe_div: " << r2.value_or(-1)
            << std::endl; // 输出 -1

  caskell::Variant<int, float> maybeInt = 42.3f;
  maybeInt.match([](int i) { std::cout << "Integer: " << i << std::endl; },
                 [](float f) { std::cout << "Float: " << f << std::endl; });

  maybeInt = 42;
  maybeInt.match([](int i) { std::cout << "Integer: " << i << std::endl; },
                 [](float f) { std::cout << "Float: " << f << std::endl; });

  std::vector<int> data = {1, 2, 3, 4, 5, 6};
}
