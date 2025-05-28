#pragma once

#include <functional>
#include <optional>
#include <tuple>

namespace caskell {

// Pattern matching base case for multiple parameters
template <typename... Ts> struct Pattern {
  virtual bool matches(const std::tuple<Ts...> &values) const = 0;
  virtual ~Pattern() = default;
};

// Value pattern for multiple parameters
template <typename... Ts> struct ValuePattern : Pattern<Ts...> {
  std::tuple<Ts...> expected;

  explicit ValuePattern(Ts... values) : expected(std::move(values)...) {}

  bool matches(const std::tuple<Ts...> &values) const override {
    return values == expected;
  }
};

// Wildcard pattern for multiple parameters
template <typename... Ts> struct WildcardPattern : Pattern<Ts...> {
  bool matches(const std::tuple<Ts...> &) const override { return true; }
};

// Guard pattern for multiple parameters
template <typename... Ts> struct GuardPattern : Pattern<Ts...> {
  std::function<bool(const Ts &...)> predicate;

  template <typename F>
  explicit GuardPattern(F &&pred) : predicate(std::forward<F>(pred)) {}

  bool matches(const std::tuple<Ts...> &values) const override {
    return std::apply(predicate, values);
  }
};

// Pattern matching expression builder for multiple parameters
template <typename R, typename... Ts> class Match {
  const std::tuple<Ts...> values;
  std::optional<R> result;

public:
  explicit Match(const Ts &...vs) : values(vs...) {}

  template <typename F>
  Match &with(const Pattern<Ts...> &pattern, F &&handler) {
    if (!result && pattern.matches(values)) {
      result = std::apply(std::forward<F>(handler), values);
    }
    return *this;
  }

  operator R() {
    if (!result) {
      throw std::runtime_error("Pattern match failed");
    }
    return *result;
  }
};

// Helper functions
template <typename R, typename... Ts> auto match(const Ts &...values) {
  return Match<R, Ts...>(values...);
}

template <typename... Ts> auto value(Ts... vs) {
  return ValuePattern<Ts...>(std::move(vs)...);
}

template <typename... Ts> auto wildcard() { return WildcardPattern<Ts...>(); }

template <typename... Ts, typename F>
auto guard(F &&pred) {
  return GuardPattern<Ts...>(std::forward<F>(pred));
}

} // namespace caskell