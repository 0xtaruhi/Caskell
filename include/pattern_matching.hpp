#pragma once

#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <any>

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
template <typename... Ts> class Match {
  const std::tuple<Ts...> values;
  std::optional<std::any> result;
  bool has_result = false;

public:
  explicit Match(Ts... vs) : values(std::make_tuple(vs...)) {}

  template <typename P, typename F> 
  Match &with(P &&pattern, F &&handler) {
    if (!has_result && pattern.matches(values)) {
      using R = std::invoke_result_t<F, Ts...>;
      result = std::apply(std::forward<F>(handler), values);
      has_result = true;
    }
    return *this;
  }

  template<typename R>
  operator R() const & {
    if (!has_result) {
      throw std::runtime_error("Pattern match failed");
    }
    return std::any_cast<R>(result);
  }

  template<typename R>
  operator R() && {
    if (!has_result) {
      throw std::runtime_error("Pattern match failed");
    }
    return std::any_cast<R>(std::move(result));
  }
};

// Helper functions
template <typename... Ts> auto match(Ts... values) {
  return Match<Ts...>(std::forward<Ts>(values)...);
}

template <typename... Ts> auto value(Ts... vs) {
  return ValuePattern<Ts...>(std::forward<Ts>(vs)...);
}

// Deduction guide for Match to help with wildcard and guard
template <typename R, typename... Ts> Match(R, Ts...) -> Match<R, R, Ts...>;

// Wildcard pattern with type deduction
template <typename M> auto wildcard_for(const M &) {
  return WildcardPattern<typename M::value_type>{};
}

// Wildcard pattern
struct WildcardMatcher {
  template <typename... Ts>
  bool matches(const std::tuple<Ts...> &) const { return true; }
};

inline auto wildcard() {
  return WildcardMatcher{};
}

// Guard pattern
template <typename F>
struct GuardMatcher {
  F predicate;

  explicit GuardMatcher(F pred) : predicate(std::move(pred)) {}

  template <typename... Ts>
  bool matches(const std::tuple<Ts...> &values) const {
    return std::apply(predicate, values);
  }
};

template <typename F> auto guard(F &&pred) {
  return GuardMatcher<std::decay_t<F>>(std::forward<F>(pred));
}

} // namespace caskell