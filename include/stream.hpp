#pragma once
#ifndef CASKELL_STREAM_HPP
#define CASKELL_STREAM_HPP

#include <memory>
#include <type_traits>
#include <utility>

namespace caskell {
namespace impl {

template <typename Container, typename NewT> struct Rebind;

template <template <typename, typename> class Container, typename T,
          typename Alloc, typename NewT>
struct Rebind<Container<T, Alloc>, NewT> {
  using type = Container<
      NewT, typename std::allocator_traits<Alloc>::template rebind_alloc<NewT>>;
};

template <template <typename> class Container, typename T, typename NewT>
struct Rebind<Container<T>, NewT> {
  using type = Container<NewT>;
};

template <typename Container, typename NewT>
using Rebind_t = typename Rebind<Container, NewT>::type;

template <typename Container> struct Stream : public Container {
  using ValueType = typename Container::value_type;

  Stream(Container &&container) : Container(std::move(container)) {}

  template <typename Pred> Stream filter(Pred pred) {
    auto slowPtr = this->begin();
    auto fastPtr = this->begin();
    auto end = this->end();
    while (fastPtr != end) {
      if (pred(*fastPtr)) {
        *slowPtr = *fastPtr;
        ++slowPtr;
      }
      ++fastPtr;
    }
    this->erase(slowPtr, end);
    return *this;
  }

  template <typename Pred> Stream filter(Pred pred) const {
    Container result;
    for (const auto &item : *this) {
      if (pred(item)) {
        result.push_back(item);
      }
    }
    return Stream(result);
  }

  template <typename Func> auto map(Func func) {
    for (auto &item : *this) {
      item = func(item);
    }
    return *this;
  }

  template <typename Func> auto map(Func func) const {
    using ReturnType = decltype(func((*this)[0]));
    using NewContainer = Rebind_t<Container, ReturnType>;
    NewContainer result;
    result.reserve(this->size());
    for (const auto &item : *this) {
      result.push_back(func(item));
    }
    return Stream(std::move(result));
  }

  template <typename Func,
            typename
            = std::enable_if_t<std::is_invocable_v<Func, ValueType, ValueType>>>
  auto reduce(Func func, ValueType &&init) const {
    auto it = std::forward<ValueType>(init);
    for (const auto &item : *this) {
      it = func(it, item);
    }
    return it;
  }

  template <typename Func,
            typename = std::enable_if_t<std::is_invocable_v<Func, ValueType>>>
  auto forEach(Func func) const {
    for (const auto &item : *this) {
      func(item);
    }
    return *this;
  }

  Container collect() && { return std::move(static_cast<Container &>(*this)); }

  Container collect() const & { return static_cast<Container>(*this); }
};
} // namespace impl

template <typename Container> auto stream(Container &&container) {
  return impl::Stream<Container>(std::forward<Container>(container));
}

} // namespace caskell

#endif // CASKELL_STREAM_HPP
