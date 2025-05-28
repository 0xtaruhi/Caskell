#pragma once
#ifndef CASKELL_MONAD_HPP
#define CASKELL_MONAD_HPP

#include "typeclass.hpp"
#include <deque>
#include <functional>
#include <string>

namespace caskell {

// Show typeclass
template <typename T> struct Show {
  static std::string show(const T &x);
};

// Identity Monad
template <typename T> class Identity {
private:
  T value;

public:
  using value_type = T;

  explicit Identity(T val) : value(std::move(val)) {}

  const T &operator*() const { return value; }
  T &operator*() { return value; }

  template <typename F> auto map(F &&f) const {
    return Identity<std::invoke_result_t<F, T>>(std::forward<F>(f)(value));
  }

  template <typename F>
  auto and_then(F &&f) const -> std::invoke_result_t<F, T> {
    return std::forward<F>(f)(value);
  }
};

// List Monad (using std::deque as the container)
template <typename T> class List {
private:
  std::deque<T> values;

public:
  using value_type = T;
  using container_type = std::deque<T>;
  using iterator = typename container_type::iterator;
  using const_iterator = typename container_type::const_iterator;

  List() = default;
  explicit List(container_type vals) : values(std::move(vals)) {}
  explicit List(T val) : values{std::move(val)} {}
  List(std::initializer_list<T> init) : values(init) {}

  const container_type &get() const { return values; }
  container_type &get() { return values; }

  // Haskell-style list operations
  bool null() const { return values.empty(); }
  T head() const { return values.front(); }
  List<T> tail() const {
    if (values.empty())
      return List<T>();
    container_type new_values(std::next(values.begin()), values.end());
    return List<T>(std::move(new_values));
  }
  List<T> init() const {
    if (values.empty())
      return List<T>();
    container_type new_values(values.begin(), std::prev(values.end()));
    return List<T>(std::move(new_values));
  }
  T last() const { return values.back(); }
  size_t length() const { return values.size(); }

  // Iterator support
  iterator begin() { return values.begin(); }
  iterator end() { return values.end(); }
  const_iterator begin() const { return values.begin(); }
  const_iterator end() const { return values.end(); }
  const_iterator cbegin() const { return values.cbegin(); }
  const_iterator cend() const { return values.cend(); }

  // cons :: a -> List a -> List a
  static List<T> cons(const T &x, const List<T> &xs) {
    auto result = xs.values;
    result.push_front(x);
    return List<T>(std::move(result));
  }

  // Operator overloading for list construction (similar to Haskell's :)
  friend List<T> operator|(const T &x, const List<T> &xs) {
    return cons(x, xs);
  }

  template <typename F> auto map(F &&f) const {
    using ResultType = std::invoke_result_t<F, T>;
    std::deque<ResultType> result;
    for (const auto &val : values) {
      result.push_back(std::forward<F>(f)(val));
    }
    return List<ResultType>(std::move(result));
  }

  template <typename F> auto and_then(F &&f) const {
    using ResultType = typename std::invoke_result_t<F, T>::value_type;
    std::deque<ResultType> result;
    for (const auto &val : values) {
      auto sublist = std::forward<F>(f)(val);
      auto &subvalues = sublist.get();
      result.insert(result.end(), subvalues.begin(), subvalues.end());
    }
    return List<ResultType>(std::move(result));
  }

  // from :: List a -> (a -> List b) -> List b
  template <typename F> auto from(F &&f) const {
    return and_then(std::forward<F>(f));
  }

  // Operator overloading for list concatenation (++)
  List<T> operator+(const List<T> &other) const {
    std::deque<T> result = values;
    result.insert(result.end(), other.values.begin(), other.values.end());
    return List<T>(std::move(result));
  }
};

// Writer Monad
template <typename T, typename W = std::string> class Writer {
private:
  T value;
  W log;

public:
  using value_type = T;
  using log_type = W;

  Writer(T val, W l) : value(std::move(val)), log(std::move(l)) {}

  std::pair<T, W> run() const { return {value, log}; }

  template <typename F> auto map(F &&f) const {
    return Writer<std::invoke_result_t<F, T>, W>(std::forward<F>(f)(value),
                                                 log);
  }

  template <typename F> auto and_then(F &&f) const {
    auto [new_value, new_log] = std::forward<F>(f)(value).run();
    return Writer<decltype(new_value), W>(
        std::move(new_value),
        log + new_log // Assumes W supports operator+
    );
  }
};

template <> struct Show<int> {
  static std::string show(const int &x) { return std::to_string(x); }
};

template <> struct Show<long> {
  static std::string show(const long &x) { return std::to_string(x); }
};

template <> struct Show<long long> {
  static std::string show(const long long &x) { return std::to_string(x); }
};

template <> struct Show<unsigned int> {
  static std::string show(const unsigned int &x) { return std::to_string(x); }
};

template <> struct Show<unsigned long> {
  static std::string show(const unsigned long &x) { return std::to_string(x); }
};

template <> struct Show<unsigned long long> {
  static std::string show(const unsigned long long &x) {
    return std::to_string(x);
  }
};

template <> struct Show<float> {
  static std::string show(const float &x) { return std::to_string(x); }
};

template <> struct Show<double> {
  static std::string show(const double &x) { return std::to_string(x); }
};

template <> struct Show<std::string> {
  static std::string show(const std::string &x) { return "\"" + x + "\""; }
};

template <typename T> struct Show<List<T>> {
  static std::string show(const List<T> &xs) {
    std::string result = "[";
    for (const auto &x : xs) {
      result += Show<T>::show(x) + ", ";
    }
    if (!xs.null()) {
      result.pop_back();
      result.pop_back();
    }
    return result + "]";
  }
};

// Specializations for Functor, Applicative, and Monad type classes

// Identity Monad
template <> struct Functor<Identity> {
  template <typename A, typename B>
  static Identity<B> fmap(Identity<A> fa, std::function<B(A)> f) {
    return fa.map(f);
  }
};

template <> struct Applicative<Identity> {
  template <typename A> static Identity<A> pure(A a) {
    return Identity<A>(std::move(a));
  }
};

template <> struct Monad<Identity> {
  template <typename A> static Identity<A> return_(A a) {
    return Identity<A>(std::move(a));
  }

  template <typename A, typename B>
  static Identity<B> bind(Identity<A> ma, std::function<Identity<B>(A)> f) {
    return ma.and_then(f);
  }
};

// List Monad
template <> struct Functor<List> {
  template <typename A, typename B>
  static List<B> fmap(List<A> fa, std::function<B(A)> f) {
    return fa.map(f);
  }
};

template <> struct Applicative<List> {
  template <typename A> static List<A> pure(A a) {
    return List<A>(std::move(a));
  }
};

template <> struct Monad<List> {
  template <typename A> static List<A> return_(A a) {
    return List<A>(std::move(a));
  }

  template <typename A, typename B>
  static List<B> bind(List<A> ma, std::function<List<B>(A)> f) {
    return ma.and_then(f);
  }
};

// Helper functions
template <typename T> Identity<T> return_identity(T value) {
  return Identity<T>(std::move(value));
}

template <typename T> List<T> return_list(T value) {
  return List<T>(std::move(value));
}

template <typename T, typename W>
Writer<T, W> return_writer(T value, W log = W{}) {
  return Writer<T, W>(std::move(value), std::move(log));
}

template <typename T, typename W> Writer<T, W> tell(W log) {
  return Writer<T, W>(T{}, std::move(log));
}

// from :: List a -> (a -> List b) -> List b
template <typename T, typename F> auto from(const List<T> &xs, F &&f) {
  return xs.from(std::forward<F>(f));
}

// range :: Int -> Int -> List Int
template <typename T> List<T> range(T start, T end) {
  std::deque<T> values;
  for (T i = start; i <= end; ++i) {
    values.push_back(i);
  }
  return List<T>(std::move(values));
}

} // namespace caskell

#endif // CASKELL_MONAD_HPP