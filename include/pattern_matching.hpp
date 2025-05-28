#pragma once

#include <functional>
#include <optional>

namespace caskell {

// Pattern matching base case
template <typename T> struct Pattern {
  virtual bool matches(const T &value) const = 0;
  virtual ~Pattern() = default;
};

// Value pattern
template <typename T> struct ValuePattern : Pattern<T> {
  T expected;

  explicit ValuePattern(T value) : expected(std::move(value)) {}

  bool matches(const T &value) const override { return value == expected; }
};

// Wildcard pattern
template <typename T> struct WildcardPattern : Pattern<T> {
  bool matches(const T &) const override { return true; }
};

// Guard pattern
template <typename T> struct GuardPattern : Pattern<T> {
  std::function<bool(const T &)> predicate;

  explicit GuardPattern(std::function<bool(const T &)> pred)
      : predicate(std::move(pred)) {}

  bool matches(const T &value) const override { return predicate(value); }
};

// Pattern matching expression builder
template <typename T, typename R> class Match {
  const T &value;
  std::optional<R> result;

public:
  explicit Match(const T &v) : value(v) {}

  Match &with(const Pattern<T> &pattern, std::function<R(const T &)> handler) {
    if (!result && pattern.matches(value)) {
      result = handler(value);
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
template <typename T, typename R> auto match(const T &value) {
  return Match<T, R>(value);
}

template <typename T> auto value(T v) { return ValuePattern<T>(std::move(v)); }

template <typename T> auto wildcard() { return WildcardPattern<T>(); }

template <typename T> auto guard(std::function<bool(const T &)> pred) {
  return GuardPattern<T>(std::move(pred));
}

} // namespace caskell