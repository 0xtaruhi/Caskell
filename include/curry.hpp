#pragma once
#ifndef CASKELL_CURRY_HPP
#define CASKELL_CURRY_HPP

#include <functional>
#include <tuple>
#include <utility>

namespace caskell {

namespace impl {

template <typename F, typename... Args> class Curried {
  F func;
  std::tuple<Args...> args;

public:
  explicit Curried(F f, std::tuple<Args...> a)
      : func(std::move(f)), args(std::move(a)) {}

  template <typename... NewArgs>
  constexpr auto operator()(NewArgs &&...newArgs) const {
    auto newTuple = std::tuple_cat(
        args, std::forward_as_tuple(std::forward<NewArgs>(newArgs)...));
    if constexpr (std::is_invocable_v<F, Args..., NewArgs...>) {
      return std::apply(
          [&](auto &&...fullArgs) {
            return std::invoke(func,
                               std::forward<decltype(fullArgs)>(fullArgs)...);
          },
          newTuple);
    } else {
      return Curried<F, Args..., NewArgs...>(func, newTuple);
    }
  }
};

} // namespace impl

template <typename F, typename... Args>
constexpr auto curry(F f, Args &&...args) {
  return impl::Curried<F, Args...>(
      std::move(f), std::forward_as_tuple(std::forward<Args>(args)...));
}
} // namespace caskell

#endif // CASKELL_CURRY_HPP
