#pragma once

#include <optional>
#include <tuple>
#include <type_traits>
#include <stdexcept>

namespace caskell {

// CRTP base class for patterns
template <typename Derived, typename... Ts> struct Pattern {
  bool matches(const std::tuple<Ts...> &values) const {
    return static_cast<const Derived *>(this)->matches_impl(values);
  }
};

// Value pattern
template <typename... Ts>
struct ValuePattern : Pattern<ValuePattern<Ts...>, Ts...> {
  std::tuple<Ts...> expected;

  explicit ValuePattern(Ts... values) : expected(std::move(values)...) {}

  bool matches_impl(const std::tuple<Ts...> &values) const {
    return values == expected;
  }
};

// Wildcard pattern
template <typename... Ts>
struct WildcardPattern : Pattern<WildcardPattern<Ts...>, Ts...> {
  bool matches_impl(const std::tuple<Ts...> &) const { return true; }
};

// Guard pattern
template <typename F, typename... Ts>
struct GuardPattern : Pattern<GuardPattern<F, Ts...>, Ts...> {
  F predicate;

  explicit GuardPattern(F pred) : predicate(std::move(pred)) {}

  bool matches_impl(const std::tuple<Ts...> &values) const {
    return std::apply(predicate, values);
  }
};

// Pattern matching expression builder
template <typename R, typename... Ts> class Match {
  const std::tuple<Ts...> values;
  std::optional<R> result;

public:
  explicit Match(Ts... vs) : values(std::make_tuple(vs...)) {}

  template <typename P, typename F> Match &with(const P &pattern, F &&handler) {
    if (!result && pattern.matches(values)) {
      result = std::apply(std::forward<F>(handler), values);
    }
    return *this;
  }

  operator R() const & {
    if (!result) {
      throw std::runtime_error("Pattern match failed");
    }
    return *result;
  }

  operator R() && {
    if (!result) {
      throw std::runtime_error("Pattern match failed");
    }
    return std::move(*result);
  }
};

// Helper functions
template <typename R, typename... Ts> auto match(Ts... values) {
  return Match<R, Ts...>(std::forward<Ts>(values)...);
}

template <typename... Ts> auto value(Ts... vs) {
  return ValuePattern<Ts...>(std::forward<Ts>(vs)...);
}

template <typename... Ts> auto wildcard() { return WildcardPattern<Ts...>(); }

template <typename... Ts, typename F> auto guard(F &&pred) {
  using GuardType = GuardPattern<std::decay_t<F>, Ts...>;
  return GuardType(std::forward<F>(pred));
}

} // namespace caskell