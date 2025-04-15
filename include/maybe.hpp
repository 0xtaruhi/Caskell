#pragma once
#ifndef CASKELL_MAYBE_HPP
#define CASKELL_MAYBE_HPP

#include <cassert>
#include <ostream>
#include <utility>

namespace caskell {

template <typename T> class Maybe {
public:
  Maybe() : has_value_(false) {}
  Maybe(const T &value) : has_value_(true), value_(value) {}
  Maybe(T &&value) : has_value_(true), value_(std::move(value)) {}

  static Maybe<T> pure(const T &value) { return Maybe<T>(value); }

  template <typename Func>
  auto map(Func func) const -> Maybe<decltype(func(std::declval<T>()))> {
    using U = decltype(func(std::declval<T>()));
    if (has_value_) {
      return Maybe<U>(func(value_));
    } else {
      return Maybe<U>();
    }
  }

  template <typename Func>
  auto and_then(Func func) const -> decltype(func(std::declval<T>())) {
    using ReturnT = decltype(func(std::declval<T>()));
    if (has_value_) {
      return func(value_);
    } else {
      return ReturnT();
    }
  }

  template <typename Func>
  auto operator>>=(Func func) const -> decltype(and_then(func)) {
    return and_then(func);
  }

  template <typename Func>
  auto operator|(Func func) const -> decltype((*this >>= func)) {
    return *this >>= func;
  }

  template <typename U> T value_or(U &&default_value) const {
    return has_value_ ? value_ : static_cast<T>(std::forward<U>(default_value));
  }

  const T &operator*() const {
    assert(has_value_);
    return value_;
  }

  bool isJust() const { return has_value_; }

  bool isNothing() const { return !has_value_; }

  friend bool operator==(const Maybe &a, const Maybe &b) {
    if (a.has_value_ != b.has_value_)
      return false;
    if (!a.has_value_)
      return true;
    return a.value_ == b.value_;
  }

  friend std::ostream &operator<<(std::ostream &os, const Maybe &m) {
    if (m.has_value_)
      return os << "Just(" << m.value_ << ")";
    return os << "Nothing";
  }

private:
  bool has_value_;
  T value_;
};

template <typename T> inline Maybe<T> pure(T value) {
  return Maybe<T>(std::move(value));
}

template <typename T> inline Maybe<T> nothing() { return Maybe<T>(); }

} // namespace caskell

#endif // CASKELL_MAYBE_HPP
