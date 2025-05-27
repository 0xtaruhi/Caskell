#pragma once
#ifndef CASKELL_TYPECLASS_HPP
#define CASKELL_TYPECLASS_HPP

#include "maybe.hpp"
#include <functional>
#include <type_traits>
#include <utility>

namespace caskell {

// Type class for Functor
template <template <typename> class F> struct Functor {
  // fmap :: (a -> b) -> f a -> f b
  template <typename A, typename B>
  static F<B> fmap(F<A> fa, std::function<B(A)> f) {
    return fa.map(f);
  }
};

// Type class for Applicative
template <template <typename> class F> struct Applicative {
  // pure :: a -> f a
  template <typename A> static F<A> pure(A a) { return F<A>(std::move(a)); }

  // <*> :: f (a -> b) -> f a -> f b
  template <typename A, typename B>
  static F<B> ap(F<std::function<B(A)>> ff, F<A> fa) {
    if constexpr (std::is_same_v<F<A>, Maybe<A>>) {
      if (!ff.isJust() || !fa.isJust()) {
        return Maybe<B>();
      }
      return Maybe<B>((*ff)(*fa));
    }
    // Implementation for other types...
    return F<B>();
  }
};

// Type class for Monad
template <template <typename> class F> struct Monad {
  // return :: a -> m a
  template <typename A> static F<A> return_(A a) { return F<A>(std::move(a)); }

  // >>= :: m a -> (a -> m b) -> m b
  template <typename A, typename B>
  static F<B> bind(F<A> ma, std::function<F<B>(A)> f) {
    if constexpr (std::is_same_v<F<A>, Maybe<A>>) {
      if (!ma.isJust()) {
        return Maybe<B>();
      }
      return f(*ma);
    }
    // Implementation for other types...
    return F<B>();
  }

  // >> :: m a -> m b -> m b
  template <typename A, typename B> static F<B> then(F<A> ma, F<B> mb) {
    auto f = std::function<F<B>(A)>([mb](A) { return mb; });
    return bind(ma, f);
  }
};

// Specialization for Maybe type
template <> struct Functor<Maybe> {
  template <typename A, typename B>
  static Maybe<B> fmap(Maybe<A> ma, std::function<B(A)> f) {
    return ma.map(f);
  }
};

template <> struct Applicative<Maybe> {
  template <typename A> static Maybe<A> pure(A a) {
    return Maybe<A>(std::move(a));
  }

  template <typename A, typename B>
  static Maybe<B> ap(Maybe<std::function<B(A)>> mf, Maybe<A> ma) {
    if (!mf.isJust() || !ma.isJust()) {
      return Maybe<B>();
    }
    return Maybe<B>((*mf)(*ma));
  }
};

template <> struct Monad<Maybe> {
  template <typename A> static Maybe<A> return_(A a) {
    return Maybe<A>(std::move(a));
  }

  template <typename A, typename B>
  static Maybe<B> bind(Maybe<A> ma, std::function<Maybe<B>(A)> f) {
    if (!ma.isJust()) {
      return Maybe<B>();
    }
    return f(*ma);
  }

  template <typename A, typename B>
  static Maybe<B> then(Maybe<A> ma, Maybe<B> mb) {
    return ma.and_then([&](auto &&) { return mb; });
  }
};

// Operator overloading namespace
namespace operators {
// Functor operator (<$>)
template <typename F, typename A,
          typename = std::enable_if_t<std::is_invocable_v<F, A>>>
auto operator>>(F &&f, const Maybe<A> &fa)
    -> Maybe<std::invoke_result_t<F, A>> {
  return fa.map(std::forward<F>(f));
}

// Applicative operator (<*>)
template <typename F, typename A,
          typename = std::enable_if_t<std::is_invocable_v<F, A>>>
auto operator*(const Maybe<F> &ff, const Maybe<A> &fa)
    -> Maybe<std::invoke_result_t<F, A>> {
  if (!ff.isJust() || !fa.isJust()) {
    return Maybe<std::invoke_result_t<F, A>>();
  }
  return Maybe<std::invoke_result_t<F, A>>((*ff)(*fa));
}

// Monad bind operator (>>=)
template <typename A, typename F, typename Result = std::invoke_result_t<F, A>,
          typename = std::enable_if_t<
              std::is_same_v<Result, Maybe<typename Result::value_type>>>>
Result operator>>(const Maybe<A> &ma, F &&f) {
  return ma.and_then(std::forward<F>(f));
}

// Monad then operator (>>)
template <typename A, typename B>
Maybe<B> operator>(const Maybe<A> &ma, const Maybe<B> &mb) {
  return ma.and_then([&](auto &&) { return mb; });
}

// Pipeline operator (|>)
template <typename A, typename F,
          typename = std::enable_if_t<std::is_invocable_v<F, A>>>
auto operator|(A &&a, F &&f) -> std::invoke_result_t<F, A> {
  return std::forward<F>(f)(std::forward<A>(a));
}
} // namespace operators

// Helper functions
template <
    template <typename> class F, typename A, typename B,
    typename = std::enable_if_t<std::is_invocable_v<std::function<B(A)>, A>>>
F<B> fmap(F<A> fa, std::function<B(A)> f) {
  return Functor<F>::fmap(fa, f);
}

template <template <typename> class F, typename A> F<A> pure(A a) {
  return Applicative<F>::pure(std::move(a));
}

template <
    template <typename> class F, typename A, typename B,
    typename = std::enable_if_t<std::is_invocable_v<std::function<B(A)>, A>>>
F<B> ap(F<std::function<B(A)>> ff, F<A> fa) {
  return Applicative<F>::ap(ff, fa);
}

template <template <typename> class F, typename A> F<A> return_(A a) {
  return Monad<F>::return_(std::move(a));
}

template <
    template <typename> class F, typename A, typename B,
    typename = std::enable_if_t<std::is_invocable_v<std::function<F<B>(A)>, A>>>
F<B> bind(F<A> ma, std::function<F<B>(A)> f) {
  return Monad<F>::bind(ma, f);
}

template <template <typename> class F, typename A, typename B>
F<B> then(F<A> ma, F<B> mb) {
  return Monad<F>::then(ma, mb);
}

} // namespace caskell

#endif // CASKELL_TYPECLASS_HPP