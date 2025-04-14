#ifndef CASKELL_LAZYSTREAM_HPP
#define CASKELL_LAZYSTREAM_HPP

#include <optional>
#include <type_traits>
#include <utility>

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
template <typename Container>
struct GeneratorTraits<ContainerGenerator<Container>> {
  using ValueType = typename ContainerGenerator<Container>::ValueType;
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
  std::optional<T> next() const {
    return static_cast<const Derived *>(this)->nextImpl();
  }
};

template <typename T>
class RangeGenerator : public Generator<RangeGenerator<T>> {
  mutable T current_;

public:
  using ValueType = T;
  explicit RangeGenerator(T start) : current_(start) {}
  std::optional<T> nextImpl() const { return current_++; }
};

template <typename Container>
class ContainerGenerator : public Generator<ContainerGenerator<Container>> {
  using Iterator = typename Container::const_iterator;
  mutable Iterator current_, end_;

public:
  using ValueType = typename Container::value_type;
  explicit ContainerGenerator(const Container &container)
      : current_(container.begin()), end_(container.end()) {}
  std::optional<ValueType> nextImpl() const {
    if (current_ == end_)
      return std::nullopt;
    return *current_++;
  }
};

template <typename Gen, typename Func>
class MapGenerator : public Generator<MapGenerator<Gen, Func>> {
  mutable Gen gen_;
  Func func_;

  using T = typename Gen::ValueType;
  using U = decltype(std::declval<Func>()(std::declval<T>()));

public:
  using ValueType = U;
  MapGenerator(Gen gen, Func func)
      : gen_(std::move(gen)), func_(std::move(func)) {}
  std::optional<U> nextImpl() const {
    if (auto item = gen_.next())
      return func_(*item);
    return std::nullopt;
  }
};

template <typename Gen, typename Pred>
class FilterGenerator : public Generator<FilterGenerator<Gen, Pred>> {
  mutable Gen gen_;
  Pred pred_;

public:
  using ValueType = typename Gen::ValueType;
  FilterGenerator(Gen gen, Pred pred)
      : gen_(std::move(gen)), pred_(std::move(pred)) {}
  std::optional<ValueType> nextImpl() const {
    while (auto item = gen_.next()) {
      if (pred_(*item))
        return item;
    }
    return std::nullopt;
  }
};

template <typename Gen>
class TakeGenerator : public Generator<TakeGenerator<Gen>> {
  mutable Gen gen_;
  mutable std::size_t remaining_;

public:
  using ValueType = typename Gen::ValueType;
  TakeGenerator(Gen gen, std::size_t n) : gen_(std::move(gen)), remaining_(n) {}
  std::optional<ValueType> nextImpl() const {
    if (remaining_ == 0)
      return std::nullopt;
    auto item = gen_.next();
    if (item) {
      --remaining_;
      return item;
    }
    return std::nullopt;
  }
};

template <typename Gen> class LazyStream {
  Gen generator_;

public:
  using ValueType = typename Gen::ValueType;

  LazyStream(Gen &&gen) : generator_(std::forward<Gen>(gen)) {}

  template <typename Func> auto map(Func &&f) const {
    using F = std::decay_t<Func>;
    using MappedGen = MapGenerator<Gen, F>;
    return LazyStream<MappedGen>(MappedGen(generator_, std::forward<Func>(f)));
  }

  template <typename Pred> auto filter(Pred &&p) const {
    using P = std::decay_t<Pred>;
    using FilteredGen = FilterGenerator<Gen, P>;
    return LazyStream<FilteredGen>(
        FilteredGen(generator_, std::forward<Pred>(p)));
  }

  auto take(std::size_t n) const {
    using TakenGen = TakeGenerator<Gen>;
    return LazyStream<TakenGen>(TakenGen(generator_, n));
  }

  template <typename U, typename Reducer> U reduce(U init, Reducer &&r) const {
    auto copy = generator_;
    while (auto item = copy.next()) {
      init = r(init, *item);
    }
    return init;
  }

  template <typename Func> void forEach(Func &&f) const {
    auto copy = generator_;
    while (auto item = copy.next()) {
      f(*item);
    }
  }

  template <typename Container> Container collect() const {
    auto copy = generator_;
    Container container;
    while (auto item = copy.next()) {
      if constexpr (impl::HasMember_push_back<Container>::value) {
        container.push_back(*item);
      } else if constexpr (impl::HasMember_insert<Container>::value) {
        container.insert(container.end(), *item);
      } else {
        static_assert(impl::HasMember_push_back<Container>::value ||
                          impl::HasMember_insert<Container>::value,
                      "Container must have push_back or insert");
      }
    }
    return container;
  }

  class Iterator {
    const Gen *generator_;
    std::optional<ValueType> current_;

  public:
    using Reference = const ValueType &;
    using Pointer = const ValueType *;
    using IteratorCategory = std::input_iterator_tag;
    using DifferenceType = std::ptrdiff_t;

    explicit Iterator(const Gen *gen)
        : generator_(gen), current_(generator_->next()) {}

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

  Iterator begin() const { return Iterator(&generator_); }
  Iterator end() const { return Iterator(); }
};

} // namespace caskell

#endif // CASKELL_LAZYSTREAM_HPP
