#include <iostream>
#include <pattern_matching.hpp>
#include <string>
#include <variant>

using namespace caskell;

// 定义一些简单的表达式类型
struct Add {
  int left;
  int right;
};

struct Sub {
  int left;
  int right;
};

struct Mul {
  int left;
  int right;
};

struct Div {
  int left;
  int right;
};

// 定义表达式类型
using Expr = std::variant<Add, Sub, Mul, Div>;

// 计算表达式的值
int eval(const Expr &expr) {
  return match(expr)
         | type<Add>() >> [](const Add &a) { return a.left + a.right; }
         | type<Sub>() >> [](const Sub &s) { return s.left - s.right; }
         | type<Mul>() >> [](const Mul &m) { return m.left * m.right; }
         | type<Div>() >> [](const Div &d) { return d.left / d.right; };
}

// 将表达式转换为字符串
std::string to_string(const Expr &expr) {
  return match(expr) | type<Add>() >> [](const Add &a) {
    return std::to_string(a.left) + " + " + std::to_string(a.right);
  } | type<Sub>() >> [](const Sub &s) {
    return std::to_string(s.left) + " - " + std::to_string(s.right);
  } | type<Mul>() >> [](const Mul &m) {
    return std::to_string(m.left) + " * " + std::to_string(m.right);
  } | type<Div>() >> [](const Div &d) {
    return std::to_string(d.left) + " / " + std::to_string(d.right);
  };
}

int main() {
  // 创建一些表达式
  Expr expr1 = Add{5, 3};
  Expr expr2 = Sub{10, 4};
  Expr expr3 = Mul{6, 7};
  Expr expr4 = Div{20, 5};

  // 测试计算
  std::cout << "Expression: " << to_string(expr1) << " = " << eval(expr1)
            << std::endl;
  std::cout << "Expression: " << to_string(expr2) << " = " << eval(expr2)
            << std::endl;
  std::cout << "Expression: " << to_string(expr3) << " = " << eval(expr3)
            << std::endl;
  std::cout << "Expression: " << to_string(expr4) << " = " << eval(expr4)
            << std::endl;

  return 0;
}