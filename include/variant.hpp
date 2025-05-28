#ifndef CASKELL_VARIANT_HPP
#define CASKELL_VARIANT_HPP

#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
namespace caskell {
namespace impl {

template <typename F> struct FunctionTraits;

template <typename Ret, typename... Args>
struct FunctionTraits<Ret (*)(Args...)> {
  using FirstArg = std::tuple_element_t<0, std::tuple<Args...>>;
};

template <typename Ret, typename C, typename... Args>
struct FunctionTraits<Ret (C::*)(Args...) const> {
  using FirstArg = std::tuple_element_t<0, std::tuple<Args...>>;
};

template <typename F> struct FirstArgType {
  using type = typename FunctionTraits<decltype(&F::operator())>::FirstArg;
};
} // namespace impl

template <typename... Ts> class Variant {
  std::variant<Ts...> data;

public:
  template <typename T> Variant(T &&value) : data(std::forward<T>(value)) {}

  template <typename... Handlers> auto match(Handlers &&...handlers) const {
    return std::visit(
        [&](auto &&arg) -> decltype(auto) {
          using T = std::decay_t<decltype(arg)>;
          (
              [&] {
                if constexpr (MatchedArm<T, Handlers>::isMatched) {
                  std::forward<Handlers>(handlers)(
                      std::forward<decltype(arg)>(arg));
                }
              }(),
              ...);
          static_assert(
              MatchedArmsCount<T, Handlers...>::count <= 1,
              "Multiple matching handlers found for the variant type");
          static_assert(MatchedArmsCount<T, Handlers...>::count > 0,
                        "No matching handler found for the variant type");
        },
        data);
  }

private:
  template <typename T, typename Handler> struct MatchedArm {
    using HandlerArg = std::decay_t<typename impl::FirstArgType<Handler>::type>;
    constexpr static bool isMatched
        = std::is_same_v<std::decay_t<T>, HandlerArg>;
  };

  template <typename T, typename... Handlers> struct MatchedArmsCount {
    static constexpr std::size_t count
        = (MatchedArm<T, Handlers>::isMatched + ...);
  };
};

} // namespace caskell

#endif // CASKELL_VARIANT_HPP
