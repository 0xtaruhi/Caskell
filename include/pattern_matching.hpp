#pragma once

#include <any>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <variant>

namespace caskell {

// Forward declarations
template <typename T> class Match;
template <typename... Ts> class MultiMatch;

// Pattern holder for operator syntax
template <typename M, typename P> class PatternHolder {
  M match;
  P pattern;

public:
  template <typename M2, typename P2>
  PatternHolder(M2 &&m, P2 &&p)
      : match(std::forward<M2>(m)), pattern(std::forward<P2>(p)) {}

  template <typename F> auto operator>>(F &&handler) {
    match.with(pattern, std::forward<F>(handler));
    return std::move(match);
  }

  template <typename P2> auto operator|(P2 &&pattern2) {
    return PatternHolder(std::move(match), std::forward<P2>(pattern2));
  }

  template <typename R> operator R() const & {
    return match.template convert<R>();
  }

  template <typename R> operator R() && {
    return std::move(match).template convert<R>();
  }
};

// Deduction guides for PatternHolder
template <typename M, typename P>
PatternHolder(M &&, P &&) -> PatternHolder<std::decay_t<M>, std::decay_t<P>>;

// Pattern function wrapper
template <typename F> struct PatternFunction {
  F func;

  explicit PatternFunction(F f) : func(std::move(f)) {}

  template <typename M> auto operator()(M &match) const { return func(match); }
};

// Type trait to check if a type is a pattern function
template <typename T> struct is_pattern_function : std::false_type {};

template <typename F>
struct is_pattern_function<PatternFunction<F>> : std::true_type {};

// Wildcard pattern (using _)
struct WildcardMatcher {
  template <typename T> bool matches(const T &) const { return true; }

  template <typename F> auto operator>>(F &&handler) const {
    return PatternFunction([handler = std::forward<F>(handler)](auto &match) {
      match.with(WildcardMatcher{}, handler);
      return match;
    });
  }
};

// Global wildcard pattern
inline const WildcardMatcher _{};

// CRTP base class for patterns
template <typename Derived, typename... Ts> struct Pattern {
  bool matches(const std::tuple<Ts...> &values) const {
    return static_cast<const Derived *>(this)->matches_impl(values);
  }

  template <typename T> bool matches(const T &value) const {
    return static_cast<const Derived *>(this)->matches_impl(value);
  }

  template <typename F> auto operator>>(F &&handler) const {
    return PatternFunction([pattern = static_cast<const Derived &>(*this),
                            handler = std::forward<F>(handler)](auto &match) {
      match.with(pattern, handler);
      return match;
    });
  }
};

// Value pattern
template <typename T> struct ValuePattern : Pattern<ValuePattern<T>, T> {
  T expected;

  explicit ValuePattern(T v) : expected(std::move(v)) {}

  bool matches_impl(const T &value) const { return value == expected; }
};

// Helper function to create value pattern
template <typename T> auto value(T v) { return ValuePattern<T>(std::move(v)); }

// Guard pattern
template <typename F> struct GuardPattern : Pattern<GuardPattern<F>> {
  F predicate;

  explicit GuardPattern(F pred) : predicate(std::move(pred)) {}

  template <typename... Ts>
  bool matches_impl(const std::tuple<Ts...> &values) const {
    return std::apply(predicate, values);
  }

  template <typename T> bool matches_impl(const T &value) const {
    if constexpr (std::is_invocable_r_v<bool, F, T>) {
      return predicate(value);
    }
    return false;
  }
};

// Helper function to create guard pattern
template <typename F> auto guard(F &&pred) {
  return GuardPattern<std::decay_t<F>>(std::forward<F>(pred));
}

// Type pattern for variant matching
template <typename T> struct TypePattern : Pattern<TypePattern<T>, T> {
  bool matches_impl(const T &) const { return true; }

  template <typename U> bool matches_impl(const U &value) const {
    if constexpr (std::is_same_v<std::decay_t<U>, std::variant<T>>) {
      return std::holds_alternative<T>(value);
    } else if constexpr (std::is_same_v<std::decay_t<U>, T>) {
      return true;
    } else {
      return false;
    }
  }

  template <typename U> T extract(const U &value) const {
    if constexpr (std::is_same_v<std::decay_t<U>, std::variant<T>>) {
      return std::get<T>(value);
    } else if constexpr (std::is_same_v<std::decay_t<U>, T>) {
      return value;
    } else {
      throw std::runtime_error("Cannot extract value from incompatible type");
    }
  }

  template <typename F> auto operator>>(F &&handler) const {
    return PatternFunction([handler = std::forward<F>(handler)](auto &match) {
      match.with(TypePattern<T>{}, [handler](const auto &value) {
        return handler(TypePattern<T>{}.extract(value));
      });
      return match;
    });
  }
};

// Helper function to create Type pattern
template <typename T> auto type() { return TypePattern<T>{}; }

// Pattern matching expression builder for single value
template <typename T> class Match {
  const T value;
  std::optional<std::any> result;
  bool has_result = false;

  // Helper to deduce the return type of a handler
  template <typename F> using HandlerReturnType = std::invoke_result_t<F, T>;

public:
  explicit Match(T v) : value(std::move(v)) {}

  template <typename P, typename F> Match &with(P &&pattern, F &&handler) {
    using R = HandlerReturnType<F>;
    if (!has_result) {
      if constexpr (std::is_invocable_r_v<bool, P, T>) {
        // Pattern is a guard function
        if (pattern(value)) {
          result = handler(value);
          has_result = true;
        }
      } else if constexpr (std::is_same_v<std::decay_t<P>, WildcardMatcher>) {
        // Pattern is a wildcard
        result = handler(value);
        has_result = true;
      } else if constexpr (std::is_convertible_v<P, T>) {
        // Pattern is a direct value
        if (value == pattern) {
          result = handler(value);
          has_result = true;
        }
      } else {
        // Pattern is a pattern type
        if (pattern.matches(value)) {
          result = handler(value);
          has_result = true;
        }
      }
    }
    return *this;
  }

  template <typename P> auto operator|(P &&pattern) {
    if constexpr (is_pattern_function<std::decay_t<P>>::value) {
      return pattern(*this);
    } else {
      return PatternHolder(*this, std::forward<P>(pattern));
    }
  }

  template <typename R> R convert() const & {
    if (!has_result) {
      throw std::runtime_error("Pattern match failed");
    }
    return std::any_cast<R>(*result);
  }

  template <typename R> R convert() && {
    if (!has_result) {
      throw std::runtime_error("Pattern match failed");
    }
    return std::any_cast<R>(std::move(*result));
  }

  template <typename R> operator R() const & { return convert<R>(); }

  template <typename R> operator R() && {
    return std::move(*this).template convert<R>();
  }
};

// Pattern matching expression builder for multiple values
template <typename... Ts> class MultiMatch {
  const std::tuple<Ts...> values;
  std::optional<std::any> result;
  bool has_result = false;

  // Helper to deduce the return type of a handler
  template <typename F>
  using HandlerReturnType = std::invoke_result_t<F, Ts...>;

public:
  explicit MultiMatch(Ts... vs) : values(std::make_tuple(vs...)) {}

  template <typename P, typename F> MultiMatch &with(P &&pattern, F &&handler) {
    using R = HandlerReturnType<F>;
    if (!has_result) {
      if constexpr (std::is_invocable_r_v<bool, P, Ts...>) {
        // Pattern is a guard function
        if (std::apply(pattern, values)) {
          result = std::apply(std::forward<F>(handler), values);
          has_result = true;
        }
      } else if constexpr (std::is_same_v<std::decay_t<P>, WildcardMatcher>) {
        // Pattern is a wildcard
        result = std::apply(std::forward<F>(handler), values);
        has_result = true;
      } else {
        // Pattern is a pattern type
        if (pattern.matches(values)) {
          result = std::apply(std::forward<F>(handler), values);
          has_result = true;
        }
      }
    }
    return *this;
  }

  template <typename P> auto operator|(P &&pattern) {
    if constexpr (is_pattern_function<std::decay_t<P>>::value) {
      return pattern(*this);
    } else {
      return PatternHolder(*this, std::forward<P>(pattern));
    }
  }

  template <typename R> R convert() const & {
    if (!has_result) {
      throw std::runtime_error("Pattern match failed");
    }
    return std::any_cast<R>(*result);
  }

  template <typename R> R convert() && {
    if (!has_result) {
      throw std::runtime_error("Pattern match failed");
    }
    return std::any_cast<R>(std::move(*result));
  }

  template <typename R> operator R() const & { return convert<R>(); }

  template <typename R> operator R() && {
    return std::move(*this).template convert<R>();
  }
};

// Pattern matching expression builder for variant
template <typename... Ts> class VariantMatch {
  const std::variant<Ts...> value;
  std::optional<std::any> result;
  bool has_result = false;

public:
  explicit VariantMatch(std::variant<Ts...> v) : value(std::move(v)) {}

  template <typename T, typename F>
  VariantMatch &with(TypePattern<T>, F &&handler) {
    if (!has_result && std::holds_alternative<T>(value)) {
      result = handler(std::get<T>(value));
      has_result = true;
    }
    return *this;
  }

  template <typename P> auto operator|(P &&pattern) {
    if constexpr (is_pattern_function<std::decay_t<P>>::value) {
      return pattern(*this);
    } else {
      return PatternHolder(*this, std::forward<P>(pattern));
    }
  }

  template <typename R> R convert() const & {
    if (!has_result) {
      throw std::runtime_error("Pattern match failed");
    }
    return std::any_cast<R>(*result);
  }

  template <typename R> R convert() && {
    if (!has_result) {
      throw std::runtime_error("Pattern match failed");
    }
    return std::any_cast<R>(std::move(*result));
  }

  template <typename R> operator R() const & { return convert<R>(); }

  template <typename R> operator R() && {
    return std::move(*this).template convert<R>();
  }
};

// Helper functions
template <typename T> auto match(T value) {
  return Match<T>(std::forward<T>(value));
}

template <typename... Ts> auto match(std::variant<Ts...> value) {
  return VariantMatch<Ts...>(std::move(value));
}

template <typename T, typename... Ts> auto match(T first, Ts... rest) {
  return MultiMatch<T, Ts...>(std::forward<T>(first),
                              std::forward<Ts>(rest)...);
}

} // namespace caskell