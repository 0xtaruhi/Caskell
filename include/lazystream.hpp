#ifndef CASKELL_LAZYSTREAM_HPP
#define CASKELL_LAZYSTREAM_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
namespace caskell {

template <typename T> class Generator {
public:
  virtual ~Generator() = default;
  virtual std::optional<T> next() = 0;
};

template <typename T, typename Func>
class MapGenerator
    : public Generator<decltype(std::declval<Func>()(std::declval<T>()))> {
  using U = decltype(std::declval<Func>()(std::declval<T>()));
  std::shared_ptr<Generator<T>> source_;
  Func func_;

public:
  MapGenerator(std::shared_ptr<Generator<T>> src, Func f)
      : source_(std::move(src)), func_(std::move(f)) {}

  std::optional<U> next() override {
    auto item = source_->next();
    if (item)
      return func_(*item);
    return std::nullopt;
  }
};

template <typename T, typename Pred>
class FilterGenerator : public Generator<T> {
  std::shared_ptr<Generator<T>> source_;
  Pred pred_;

public:
  FilterGenerator(std::shared_ptr<Generator<T>> src, Pred p)
      : source_(std::move(src)), pred_(std::move(p)) {}

  std::optional<T> next() override {
    while (auto item = source_->next()) {
      if (pred_(*item))
        return item;
    }
    return std::nullopt;
  }
};

template <typename T> class TakeGenerator : public Generator<T> {
  std::shared_ptr<Generator<T>> source_;
  std::size_t remaining_;

public:
  TakeGenerator(std::shared_ptr<Generator<T>> src, std::size_t n)
      : source_(std::move(src)), remaining_(n) {}

  std::optional<T> next() override {
    if (remaining_ == 0)
      return std::nullopt;
    auto item = source_->next();
    if (item) {
      --remaining_;
      return item;
    }
    return std::nullopt;
  }
};

template <typename T> class RangeGenerator : public Generator<T> {
  T current_;
  size_t step_;

public:
  explicit RangeGenerator(T start) : current_(start) {}

  std::optional<T> next() override { return current_++; }
};

template <typename T> class ContainerGenerator : public Generator<T> {
  using Iterator = typename std::vector<T>::const_iterator;
  Iterator current_, end_;

public:
  ContainerGenerator(Iterator begin, Iterator end)
      : current_(begin), end_(end) {}

  std::optional<T> next() override {
    if (current_ == end_)
      return std::nullopt;
    return *current_++;
  }
};

// LazyStream wrapper
template <typename T> class LazyStream {
  std::shared_ptr<Generator<T>> generator_;

  explicit LazyStream(std::shared_ptr<Generator<T>> gen)
      : generator_(std::move(gen)) {}

public:
  static LazyStream<T> fromRange(T start) {
    return LazyStream<T>(std::make_shared<RangeGenerator<T>>(start));
  }

  static LazyStream<T> fromContainer(const std::vector<T> &container) {
    return LazyStream<T>(std::make_shared<ContainerGenerator<T>>(
        container.begin(), container.end()));
  }

  template <typename Func> auto map(Func f) const {
    using U = decltype(f(std::declval<T>()));
    auto gen =
        std::make_shared<MapGenerator<T, Func>>(generator_, std::move(f));
    return LazyStream<U>(gen);
  }

  template <typename Pred> auto filter(Pred p) const {
    auto gen =
        std::make_shared<FilterGenerator<T, Pred>>(generator_, std::move(p));
    return LazyStream<T>(gen);
  }

  auto take(std::size_t n) const {
    auto gen = std::make_shared<TakeGenerator<T>>(generator_, n);
    return LazyStream<T>(gen);
  }

  template <typename CollectContainer = std::vector<T>>
  CollectContainer collect() const {
    CollectContainer result;
    while (auto item = generator_->next()) {
      result.push_back(std::move(*item));
    }
    return result;
  }

  template <typename U, typename Reducer> U reduce(U init, Reducer r) const {
    while (auto item = generator_->next()) {
      init = r(init, *item);
    }
    return init;
  }

  template <typename Func> void forEach(Func f) const {
    while (auto item = generator_->next()) {
      f(*item);
    }
  }

  using iterator = typename std::shared_ptr<Generator<T>>::element_type;

  auto begin() { return generator_; }
};

} // namespace caskell

#endif // CASKELL_LAZYSTREAM_HPP
