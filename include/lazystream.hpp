#ifndef CASKELL_LAZYSTREAM_HPP
#define CASKELL_LAZYSTREAM_HPP

#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace caskell {

namespace impl {
template <typename T, typename = void>
struct HasMember_push_back : std::false_type {};

template <typename T>
struct HasMember_push_back<T,
                           std::void_t<decltype(std::declval<T &>().push_back(
                               std::declval<typename T::value_type>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct HasMember_insert : std::false_type {};

template <typename T>
struct HasMember_insert<T, std::void_t<decltype(std::declval<T &>().insert(
                               std::declval<typename T::iterator>(),
                               std::declval<typename T::value_type>()))>>
    : std::true_type {};
} // namespace impl

template <typename T> class RangeGenerator;
template <typename T> class ContainerGenerator;
template <typename Gen, typename Func> class MapGenerator;
template <typename Gen, typename Pred> class FilterGenerator;
template <typename Gen> class TakeGenerator;

namespace impl {
template <typename T> struct GeneratorTraits;
template <typename T> struct GeneratorTraits<RangeGenerator<T>> {
  using ValueType = T;
};
template <typename T> struct GeneratorTraits<ContainerGenerator<T>> {
  using ValueType = T;
};

template <typename Gen, typename Func>
struct GeneratorTraits<MapGenerator<Gen, Func>> {
  using SourceT = typename GeneratorTraits<Gen>::ValueType;
  using ValueType = decltype(std::declval<Func>()(std::declval<SourceT>()));
};

template <typename Gen, typename Pred>
struct GeneratorTraits<FilterGenerator<Gen, Pred>> {
  using ValueType = typename Gen::ValueType;
};

template <typename Gen> struct GeneratorTraits<TakeGenerator<Gen>> {
  using ValueType = typename Gen::ValueType;
};
} // namespace impl

template <typename Derived> class Generator {
public:
  using T = typename impl::GeneratorTraits<Derived>::ValueType;
  std::optional<T> next() { return static_cast<Derived *>(this)->nextImpl(); }
};

template <typename Gen, typename Func>
class MapGenerator : public Generator<MapGenerator<Gen, Func>> {
  Gen gen_;
  Func func_;

  using T = typename Gen::ValueType;
  using U = decltype(std::declval<Func>()(std::declval<T>()));

public:
  using ValueType = U;
  std::optional<U> nextImpl() {
    if (auto item = gen_.next()) {
      return func_(*item);
    }
    return std::nullopt;
  }

  MapGenerator(Gen gen, Func func)
      : gen_(std::move(gen)), func_(std::move(func)) {}
};

template <typename Gen, typename Pred>
class FilterGenerator : public Generator<FilterGenerator<Gen, Pred>> {
  Gen gen_;
  Pred pred_;

public:
  using ValueType = typename Gen::ValueType;
  std::optional<ValueType> nextImpl() {
    while (auto item = gen_.next()) {
      if (pred_(*item))
        return item;
    }
    return std::nullopt;
  }

  FilterGenerator(Gen gen, Pred pred)
      : gen_(std::move(gen)), pred_(std::move(pred)) {}
};

template <typename Gen>
class TakeGenerator : public Generator<TakeGenerator<Gen>> {
  Gen gen_;
  std::size_t remaining_;

public:
  using ValueType = typename Gen::ValueType;
  std::optional<ValueType> nextImpl() {
    if (remaining_ == 0)
      return std::nullopt;
    auto item = gen_.next();
    if (item) {
      --remaining_;
      return item;
    }
    return std::nullopt;
  }

  TakeGenerator(Gen gen, std::size_t n) : gen_(std::move(gen)), remaining_(n) {}
};

template <typename T>
class RangeGenerator : public Generator<RangeGenerator<T>> {
  T current_;

public:
  using ValueType = T;
  explicit RangeGenerator(T start) : current_(start) {}

  std::optional<T> nextImpl() { return current_++; }
};

template <typename T>
class ContainerGenerator : public Generator<ContainerGenerator<T>> {
  using Iterator = typename std::vector<T>::const_iterator;
  Iterator current_, end_;

public:
  using ValueType = T;
  ContainerGenerator(Iterator begin, Iterator end)
      : current_(begin), end_(end) {}

  std::optional<T> nextImpl() {
    if (current_ == end_)
      return std::nullopt;
    return *current_++;
  }
};

template <typename Gen> class LazyStream {
  Gen generator_;

public:
  explicit LazyStream(Gen gen) : generator_(std::move(gen)) {}
  using ValueType = typename Gen::ValueType;

  template <typename Func> auto map(Func f) const {
    using MappedGen = MapGenerator<Gen, Func>;
    return LazyStream<MappedGen>(MappedGen{generator_, std::move(f)});
  }

  template <typename Pred> auto filter(Pred p) const {
    using FilteredGen = FilterGenerator<Gen, Pred>;
    return LazyStream<FilteredGen>(FilteredGen{generator_, std::move(p)});
  }

  auto take(std::size_t n) const {
    using TakenGen = TakeGenerator<Gen>;
    return LazyStream<TakenGen>(TakenGen{generator_, n});
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

  auto next() { return generator_.next(); }

  class Iterator {
    Gen *generator_;
    std::optional<ValueType> current_;

  public:
    using Reference = const ValueType &;
    using Pointer = const ValueType *;
    using IteratorCategory = std::input_iterator_tag;
    using DifferenceType = std::ptrdiff_t;

    explicit Iterator(Gen *gen)
        : generator_(std::move(gen)), current_(generator_->next()) {}

    Iterator() : generator_(nullptr), current_(std::nullopt) {}

    Reference operator*() const { return *current_; }
    Pointer operator->() const { return &*current_; }

    Iterator &operator++() {
      current_ = generator_->next();
      return *this;
    }

    bool operator==(const Iterator &other) const {
      return (!current_ && !other.current_);
    }

    bool operator!=(const Iterator &other) const { return !(*this == other); }
  };

  Iterator begin() { return Iterator(&generator_); }
  Iterator end() { return Iterator(); }
};

} // namespace caskell

#endif // CASKELL_LAZYSTREAM_HPP
