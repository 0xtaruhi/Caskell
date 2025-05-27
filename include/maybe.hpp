#pragma once
#ifndef CASKELL_MAYBE_HPP
#define CASKELL_MAYBE_HPP

#include <cassert>
#include <optional>
#include <ostream>
#include <type_traits>
#include <utility>

namespace caskell {

// Maybe type implementation
template <typename T> class Maybe {
private:
  std::optional<T> value;

public:
  using value_type = T;

  // Constructors
  Maybe() = default;
  explicit Maybe(T value) : value(std::move(value)) {}

  // Check if Maybe contains a value
  bool isJust() const { return value.has_value(); }
  bool isNothing() const { return !value.has_value(); }

  // Safe value access
  const T &operator*() const { return *value; }
  T &operator*() { return *value; }

  // Map operation (fmap)
  template <typename F, typename = std::enable_if_t<std::is_invocable_v<F, T>>>
  auto map(F &&f) const {
    if (!isJust()) {
      return Maybe<std::invoke_result_t<F, T>>();
    }
    return Maybe<std::invoke_result_t<F, T>>(std::forward<F>(f)(*value));
  }

  // Bind operation (>>=)
  template <typename F, typename Result = std::invoke_result_t<F, T>,
            typename = std::enable_if_t<
                std::is_same_v<Result, Maybe<typename Result::value_type>>>>
  Result and_then(F &&f) const {
    if (!isJust()) {
      return Result();
    }
    return std::forward<F>(f)(*value);
  }

  // Value or default
  template <typename U> T value_or(U &&default_value) const {
    return value.value_or(std::forward<U>(default_value));
  }

  // Operator overloads
  template <typename F, typename = std::enable_if_t<std::is_invocable_v<F, T>>>
  auto operator>>(F &&f) const {
    return map(std::forward<F>(f));
  }

  template <typename F, typename Result = std::invoke_result_t<F, T>,
            typename = std::enable_if_t<
                std::is_same_v<Result, Maybe<typename Result::value_type>>>>
  Result operator>>=(F &&f) const {
    return and_then(std::forward<F>(f));
  }

  template <typename F, typename = std::enable_if_t<std::is_invocable_v<F, T>>>
  auto operator|(F &&f) const {
    if (!isJust()) {
      return std::invoke_result_t<F, T>();
    }
    return std::forward<F>(f)(*value);
  }

  friend bool operator==(const Maybe &a, const Maybe &b) {
    if (a.value.has_value() != b.value.has_value())
      return false;
    if (!a.value.has_value())
      return true;
    return a.value.value() == b.value.value();
  }

  friend std::ostream &operator<<(std::ostream &os, const Maybe &m) {
    if (m.value.has_value())
      return os << "Just(" << m.value.value() << ")";
    return os << "Nothing";
  }
};

template <typename T> inline Maybe<T> pure(T value) {
  return Maybe<T>(std::move(value));
}

template <typename T> inline Maybe<T> nothing() { return Maybe<T>(); }

} // namespace caskell

#endif // CASKELL_MAYBE_HPP
