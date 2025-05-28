#pragma once
#ifndef CASKELL_UTILS_HPP
#define CASKELL_UTILS_HPP

#include <utility>

namespace caskell {

// Y combinator for recursive lambdas
template <typename F> struct y_combinator {
  F f;
  template <typename... Args> decltype(auto) operator()(Args &&...args) const {
    return f(*this, std::forward<Args>(args)...);
  }
};

template <typename F> y_combinator<std::decay_t<F>> fix(F &&f) {
  return {std::forward<F>(f)};
}

// make_y_combinator :: (a -> b) -> (a -> b)
template <typename F> auto make_y_combinator(F &&f) {
  return fix(std::forward<F>(f));
}

} // namespace caskell

#endif // CASKELL_UTILS_HPP