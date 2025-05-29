#include "caskell.hpp"
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

using namespace caskell;

struct Expr;
using ExprPtr = std::shared_ptr<Expr>;

struct Var {
  std::string name;
};
struct Const {
  double value;
};
struct Add {
  ExprPtr l, r;
};
struct Mul {
  ExprPtr l, r;
};
struct Sub {
  ExprPtr l, r;
};
struct Div {
  ExprPtr l, r;
};
struct Pow {
  ExprPtr base;
  double exp;
};
struct Sin {
  ExprPtr arg;
};
struct Cos {
  ExprPtr arg;
};
struct Exp {
  ExprPtr arg;
};

struct Expr
    : public Variant<Var, Const, Add, Mul, Sub, Div, Pow, Sin, Cos, Exp> {
  using Variant::Variant;
};

ExprPtr var(const std::string &name) {
  return std::make_shared<Expr>(Var{name});
}
ExprPtr cnst(double v) { return std::make_shared<Expr>(Const{v}); }
ExprPtr add(ExprPtr l, ExprPtr r) { return std::make_shared<Expr>(Add{l, r}); }
ExprPtr mul(ExprPtr l, ExprPtr r) { return std::make_shared<Expr>(Mul{l, r}); }
ExprPtr sub(ExprPtr l, ExprPtr r) { return std::make_shared<Expr>(Sub{l, r}); }
ExprPtr div(ExprPtr l, ExprPtr r) { return std::make_shared<Expr>(Div{l, r}); }
ExprPtr pow(ExprPtr base, double exp) {
  return std::make_shared<Expr>(Pow{base, exp});
}
ExprPtr sin(ExprPtr arg) { return std::make_shared<Expr>(Sin{arg}); }
ExprPtr cos(ExprPtr arg) { return std::make_shared<Expr>(Cos{arg}); }
ExprPtr exp(ExprPtr arg) { return std::make_shared<Expr>(Exp{arg}); }

bool is_const(const ExprPtr &e) {
  bool result = false;
  e->match([&result](const Var &) -> void { result = false; },
           [&result](const Const &) -> void { result = true; },
           [&result](const Add &) -> void { result = false; },
           [&result](const Mul &) -> void { result = false; },
           [&result](const Sub &) -> void { result = false; },
           [&result](const Div &) -> void { result = false; },
           [&result](const Pow &) -> void { result = false; },
           [&result](const Sin &) -> void { result = false; },
           [&result](const Cos &) -> void { result = false; },
           [&result](const Exp &) -> void { result = false; });
  return result;
}

double get_const_value(const ExprPtr &e) {
  double value = 0;
  e->match([&value](const Var &) -> void { value = 0; },
           [&value](const Const &c) -> void { value = c.value; },
           [&value](const Add &) -> void { value = 0; },
           [&value](const Mul &) -> void { value = 0; },
           [&value](const Sub &) -> void { value = 0; },
           [&value](const Div &) -> void { value = 0; },
           [&value](const Pow &) -> void { value = 0; },
           [&value](const Sin &) -> void { value = 0; },
           [&value](const Cos &) -> void { value = 0; },
           [&value](const Exp &) -> void { value = 0; });
  return value;
}

ExprPtr simplify(const ExprPtr &e) {
  if (e == nullptr)
    return nullptr;
  ExprPtr result;
  e->match([&result](const Var &v) { result = var(v.name); },
           [&result](const Const &c) { result = cnst(c.value); },

           [&result](const Add &a) {
             auto l = simplify(a.l);
             auto r = simplify(a.r);

             if (is_const(l) && get_const_value(l) == 0.0) {
               result = r;
               return;
             }

             if (is_const(r) && get_const_value(r) == 0.0) {
               result = l;
               return;
             }
             if (is_const(l) && is_const(r)) {
               result = cnst(get_const_value(l) + get_const_value(r));
               return;
             }
             result = add(l, r);
           },

           [&result](const Sub &s) {
             auto l = simplify(s.l);
             auto r = simplify(s.r);
           },

           [&result](const Mul &m) {
             auto l = simplify(m.l);
             auto r = simplify(m.r);

             if (is_const(l) && get_const_value(l) == 0.0) {
               result = cnst(0.0);
               return;
             }

             if (is_const(r) && get_const_value(r) == 0.0) {
               result = cnst(0.0);
               return;
             }

             if (is_const(l) && get_const_value(l) == 1.0) {
               result = r;
               return;
             }

             if (is_const(r) && get_const_value(r) == 1.0) {
               result = l;
               return;
             }

             if (is_const(l) && is_const(r)) {
               result = cnst(get_const_value(l) * get_const_value(r));
               return;
             }
             result = mul(l, r);
           },

           [&result](const Div &d) {
             auto l = simplify(d.l);
             auto r = simplify(d.r);

             if (is_const(l) && get_const_value(l) == 0.0) {
               result = cnst(0.0);
               return;
             }

             if (is_const(r) && get_const_value(r) == 1.0) {
               result = l;
               return;
             }

             if (is_const(l) && is_const(r)) {
               result = cnst(get_const_value(l) / get_const_value(r));
               return;
             }
             result = div(l, r);
           },

           [&result](const Pow &p) {
             auto b = simplify(p.base);

             if (p.exp == 0) {
               result = cnst(1.0);
               return;
             }

             if (p.exp == 1) {
               result = b;
               return;
             }

             if (p.exp > 0 && is_const(b) && get_const_value(b) == 0.0) {
               result = cnst(0.0);
               return;
             }

             if (is_const(b) && get_const_value(b) == 1.0) {
               result = cnst(1.0);
               return;
             }
             result = pow(b, p.exp);
           },

           [&result](const Sin &s) {
             auto arg = simplify(s.arg);
             if (is_const(arg)) {
               result = cnst(std::sin(get_const_value(arg)));
               return;
             }
             result = sin(arg);
           },
           [&result](const Cos &c) {
             auto arg = simplify(c.arg);
             if (is_const(arg)) {
               result = cnst(std::cos(get_const_value(arg)));
               return;
             }
             result = cos(arg);
           },
           [&result](const Exp &e) {
             auto arg = simplify(e.arg);
             if (is_const(arg)) {
               result = cnst(std::exp(get_const_value(arg)));
               return;
             }
             result = exp(arg);
           });
  return result;
}

namespace {
constexpr int PREC_CONST = 100; // Constants and variables
constexpr int PREC_FUNC = 90;   // Function calls (sin, cos, exp)
constexpr int PREC_POW = 80;    // Power operator
constexpr int PREC_MUL = 70;    // Multiplication
constexpr int PREC_DIV = 70;    // Division
constexpr int PREC_ADD = 60;    // Addition
constexpr int PREC_SUB = 60;    // Subtraction
} // namespace

template <> struct Show<Expr> {

  std::string wrap_if_needed(const Expr &e, int parent_prec) const {
    std::string result = operator()(e);
    if (get_precedence(e) < parent_prec) {
      result = "(" + result + ")";
    }
    return result;
  }

  int get_precedence(const Expr &e) const {
    int prec = PREC_CONST;
    e.match([&prec](const Var &) { prec = PREC_CONST; },
            [&prec](const Const &) { prec = PREC_CONST; },
            [&prec](const Add &) { prec = PREC_ADD; },
            [&prec](const Mul &) { prec = PREC_MUL; },
            [&prec](const Div &) { prec = PREC_DIV; },
            [&prec](const Sub &) { prec = PREC_SUB; },
            [&prec](const Pow &) { prec = PREC_POW; },
            [&prec](const Sin &) { prec = PREC_FUNC; },
            [&prec](const Cos &) { prec = PREC_FUNC; },
            [&prec](const Exp &) { prec = PREC_FUNC; });
    return prec;
  }

  std::string operator()(const Expr &e) const {
    std::string result;
    e.match([&result](const Var &v) { result = v.name; },
            [&result](const Const &c) { result = std::to_string(c.value); },
            [&result, this](const Add &a) {
              result = wrap_if_needed(*a.l, PREC_ADD) + " + "
                       + wrap_if_needed(*a.r, PREC_ADD);
            },
            [&result, this](const Sub &s) {
              result = wrap_if_needed(*s.l, PREC_SUB) + " - "
                       + wrap_if_needed(*s.r, PREC_SUB);
            },
            [&result, this](const Mul &m) {
              result = wrap_if_needed(*m.l, PREC_MUL) + " * "
                       + wrap_if_needed(*m.r, PREC_MUL);
            },
            [&result, this](const Div &d) {
              result = wrap_if_needed(*d.l, PREC_DIV) + " / "
                       + wrap_if_needed(*d.r, PREC_DIV);
            },
            [&result, this](const Pow &p) {
              result = wrap_if_needed(*p.base, PREC_POW) + "^"
                       + std::to_string(p.exp);
            },
            [&result, this](const Sin &s) {
              result = "sin(" + operator()(*s.arg) + ")";
            },
            [&result, this](const Cos &c) {
              result = "cos(" + operator()(*c.arg) + ")";
            },
            [&result, this](const Exp &e) {
              result = "exp(" + operator()(*e.arg) + ")";
            });
    return result;
  }
};

ExprPtr derivative(const ExprPtr &e, const std::string &v) {
  ExprPtr result;
  e->match([&result,
            &v](const Var &var_) { result = cnst(var_.name == v ? 1.0 : 0.0); },
           [&result](const Const &) { result = cnst(0.0); },
           [&result, &v](const Add &a) {
             result = add(derivative(a.l, v), derivative(a.r, v));
           },
           [&result, &v](const Sub &s) {
             // (f - g)' = f' - g'
             result = sub(derivative(s.l, v), derivative(s.r, v));
           },
           [&result, &v](const Mul &m) {
             // (f * g)' = f' * g + f * g'
             result = add(mul(derivative(m.l, v), m.r),
                          mul(m.l, derivative(m.r, v)));
           },
           [&result, &v](const Div &d) {
             // (f / g)' = (f' * g - f * g') / g^2
             result = div(sub(mul(derivative(d.l, v), d.r),
                              mul(d.l, derivative(d.r, v))),
                          pow(d.r, 2));
           },
           [&result, &v](const Pow &p) {
             // (f^n)' = n * f' * f^(n-1)
             result = mul(cnst(p.exp),
                          mul(derivative(p.base, v), pow(p.base, p.exp - 1)));
           },
           [&result, &v](const Sin &s) {
             // (sin(f))' = cos(f) * f'
             result = mul(cos(s.arg), derivative(s.arg, v));
           },
           [&result, &v](const Cos &c) {
             // (cos(f))' = -sin(f) * f'
             result = mul(mul(cnst(-1.0), sin(c.arg)), derivative(c.arg, v));
           },
           [&result, &v](const Exp &e) {
             // (e^f)' = e^f * f'
             result = mul(exp(e.arg), derivative(e.arg, v));
           });
  return simplify(result); // Simplify the derivative result
}

int main() {

  auto x = var("x");
  auto expr = add(mul(sin(pow(x, 2)), cos(x)), exp(x));

  auto d = derivative(expr, "x");

  std::cout << "Original function: " << Show<Expr>()(*expr) << std::endl;
  std::cout << "Derivative:       " << Show<Expr>()(*d) << std::endl;

  auto expr2 = cos(exp(cos(mul(cnst(2), add(cnst(1), pow(x, 3))))));
  auto d2 = derivative(expr2, "x");

  std::cout << "\nMore complex test:" << std::endl;
  std::cout << "Original: " << Show<Expr>()(*expr2) << std::endl;
  std::cout << "Derivative: " << Show<Expr>()(*d2) << std::endl;

  return 0;
}
