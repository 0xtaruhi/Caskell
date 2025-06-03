#include "caskell.hpp"
#include <iostream>

// pattern_matching is not quite useful, bugs still exist

using namespace caskell;

using Item = std::pair<int, int>;
using KnapsackResult = std::pair<int, List<int>>;

const auto safeGetItem
    = curry([](const List<Item> &items, int idx) -> Maybe<Item> {
        return match(std::make_pair(idx, items.length()))
               | (_ >> [&items, idx](const auto &) {
                   return (idx >= 0 && idx < items.length())
                              ? pure(items.get()[idx])
                              : nothing<Item>();
                 });
      });

const auto reverseList = [](const List<Item> &items) -> List<Item> {
  List<Item> result;
  for (int i = items.length() - 1; i >= 0; --i) {
    result = List<Item>::cons(items.get()[i], result);
  }
  return result;
};

const auto knapsack = curry([](int capacity,
                               const List<Item> &items) -> KnapsackResult {
  const auto knapsackRec = make_y_combinator([](auto self, int cap,
                                                const List<Item> &its,
                                                const List<int> &selected,
                                                int currentIndex)
                                                 -> KnapsackResult {
    return match(std::make_tuple(its, cap))
           | (value(std::make_tuple(List<Item>(), 0)) >>
              [selected](const auto &) { return KnapsackResult{0, selected}; })
           | (value(std::make_tuple(List<Item>(), _)) >>
              [selected](const auto &) { return KnapsackResult{0, selected}; })
           | (_ >> [selected, &self, currentIndex](const auto &tuple) {
               const auto &its = std::get<0>(tuple);
               const auto &cap = std::get<1>(tuple);
               const auto current = its.head();
               const auto rest = its.tail();

               return match(std::make_pair(cap, current.first))
                      // not support (0, _) compile failed
                      | (value(std::make_pair(0, _)) >>
                         [selected](const auto &) {
                           return KnapsackResult{0, selected};
                         })
                      | (_ >> [cap, current, rest, currentIndex, selected,
                               &self](const auto &pair) {
                          const auto capacity = pair.first;
                          const auto weight = pair.second;
                          return weight > capacity
                                     ? self(capacity, rest, selected,
                                            currentIndex + 1)
                                     : match(std::make_pair(
                                           self(capacity, rest, selected,
                                                currentIndex + 1),
                                           self(capacity - weight, rest,
                                                currentIndex | selected,
                                                currentIndex + 1)))
                                           | (_ >> [current](
                                                       const auto &results) {
                                               const auto &[without, with]
                                                   = results;
                                               const auto withValue
                                                   = current.second
                                                     + with.first;
                                               return withValue > without.first
                                                          ? KnapsackResult{withValue,
                                                                           with.second}
                                                          : without;
                                             });
                        });
             });
  });

  return knapsackRec(capacity, reverseList(items), List<int>(), 0);
});

const auto calculateTotals
    = curry([](const List<Item> &items,
               const List<int> &selected) -> std::pair<int, int> {
        auto result = std::make_pair(0, 0);
        for (const auto &idx : selected) {
          safeGetItem(items)(idx).map([&result](const Item &item) {
            result.first += item.first;
            result.second += item.second;
            return item;
          });
        }
        return result;
      });

const auto formatResult
    = curry([](const KnapsackResult &result,
               const List<Item> &items) -> List<std::string> {
        const auto totals = calculateTotals(items)(result.second);

        std::string indices;
        bool first = true;
        for (const auto &idx : result.second) {
          if (!first)
            indices += " ";
          indices += std::to_string(idx);
          first = false;
        }

        return List<std::string>(
            {"Max Value: " + std::to_string(result.first),
             "Selected Item: " + indices,
             "Total Weight: " + std::to_string(totals.first),
             "Total Value: " + std::to_string(totals.second)});
      });

int main() {
  const auto items
      = List<Item>({{2, 3}, {3, 4}, {4, 5}, {5, 6}, {6, 7}});

  const int capacity = 12;

  const auto result = knapsack(capacity)(items);
  formatResult(result)(items).map([](const std::string &line) {
    std::cout << line << std::endl;
    return line;
  });

  return 0;
}