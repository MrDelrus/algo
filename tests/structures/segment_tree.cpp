// Correctness tests for algo::data_structures::segment_tree.
//
// Two layers. The small layer is exhaustive: every size from 0 to 32, every range, every
// position, every starting point for the descent, all compared against a linear fold. Sizes
// around powers of two get particular attention, because the leaf count is padded there and an
// off-by-one in the padding would hide in exactly those sizes. The large layer runs sizes near
// 1e3 from a deterministic generator and checks each answer against an obvious O(n) scan.
//
// Everything is deterministic: fixed-seed counter-based splitmix64, no std distribution, so a
// failure reproduces byte for byte on any machine.

#include "algo_library.hpp"

#include "../harness.hpp"

#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

namespace {

using testing::check;
using testing::check_does_not_throw;
using testing::check_equal;
using testing::check_throws;

constexpr std::size_t exhaustive_limit = 32;
constexpr std::uint64_t seed = 0xcbf29ce484222325ULL;

// Non-commutative on purpose: string concatenation makes any slip in operand order visible,
// which sum, min, max and gcd all hide.
struct concat_monoid {
  using value_type = std::string;
  static value_type identity() {
    return std::string();
  }
  static value_type combine(const value_type& left, const value_type& right) {
    return left + right;
  }
};

// The obvious O(n) fold. Everything below is checked against this and nothing else.
template <typename monoid>
typename monoid::value_type linear_fold(const std::vector<typename monoid::value_type>& values,
                                        std::size_t left, std::size_t right) {
  typename monoid::value_type folded = monoid::identity();
  for (std::size_t index = left; index < right; ++index) {
    folded = monoid::combine(folded, values[index]);
  }
  return folded;
}

// The obvious O(n^2) descent: grow the range one element at a time until the predicate breaks.
template <typename monoid, typename predicate>
std::size_t linear_find_right(const std::vector<typename monoid::value_type>& values,
                              std::size_t left, predicate is_good) {
  std::size_t answer = left;
  for (std::size_t right = left; right <= values.size(); ++right) {
    if (!is_good(linear_fold<monoid>(values, left, right))) {
      break;
    }
    answer = right;
  }
  return answer;
}

template <typename monoid, typename predicate>
std::size_t linear_find_left(const std::vector<typename monoid::value_type>& values,
                             std::size_t right, predicate is_good) {
  std::size_t answer = right;
  for (std::size_t left = right + 1; left-- > 0;) {
    if (!is_good(linear_fold<monoid>(values, left, right))) {
      break;
    }
    answer = left;
  }
  return answer;
}

std::string describe(std::size_t size, std::size_t first, std::size_t second) {
  return "n=" + std::to_string(size) + " [" + std::to_string(first) + ", " +
         std::to_string(second) + ")";
}

// Every range, every position, against the linear fold.
template <typename monoid>
void check_against_linear(const algo::data_structures::segment_tree<monoid>& tree,
                          const std::vector<typename monoid::value_type>& values,
                          const std::string& note) {
  std::size_t size = values.size();
  for (std::size_t left = 0; left <= size; ++left) {
    for (std::size_t right = left; right <= size; ++right) {
      check_equal(tree.query(left, right) == linear_fold<monoid>(values, left, right), true,
                  note + " query " + describe(size, left, right));
    }
  }
  for (std::size_t position = 0; position < size; ++position) {
    check(tree.get(position) == values[position],
          note + " get " + std::to_string(position) + " of " + std::to_string(size));
  }
  check(tree.query_all() == linear_fold<monoid>(values, 0, size), note + " query_all");
  check(tree.query_all() == tree.query(0, size), note + " query_all equals query(0, n)");
}

template <typename monoid, typename value_generator, typename predicate_generator>
void exhaustive_small(const std::string& name, value_generator make_value,
                      predicate_generator make_predicate) {
  testing::section(name + " / exhaustive n <= " + std::to_string(exhaustive_limit));
  testing::random_source source(seed);

  for (std::size_t size = 0; size <= exhaustive_limit; ++size) {
    std::vector<typename monoid::value_type> values;
    for (std::size_t index = 0; index < size; ++index) {
      values.push_back(make_value(source));
    }

    algo::data_structures::segment_tree<monoid> tree(values);
    check_against_linear<monoid>(tree, values, name + " fresh");

    // Both update flavours, with the whole tree re-verified after each one. Slow on purpose:
    // at these sizes a full re-check costs nothing and catches an ancestor that was updated
    // along the wrong path.
    for (std::size_t round = 0; round < 3 * size; ++round) {
      std::size_t position = source.below(size);
      typename monoid::value_type value = make_value(source);
      if (source.next() % 2 == 0) {
        tree.set(position, value);
        values[position] = value;
      } else {
        tree.combine_at(position, value);
        values[position] = monoid::combine(values[position], value);
      }
      check_against_linear<monoid>(tree, values, name + " after update");
    }

    // The descent, from every starting point, against the linear scan.
    for (std::size_t attempt = 0; attempt < 4; ++attempt) {
      auto is_good = make_predicate(source);
      for (std::size_t left = 0; left <= size; ++left) {
        check_equal(
            tree.find_right(left, is_good), linear_find_right<monoid>(values, left, is_good),
            name + " find_right n=" + std::to_string(size) + " left=" + std::to_string(left));
      }
      for (std::size_t right = 0; right <= size; ++right) {
        check_equal(
            tree.find_left(right, is_good), linear_find_left<monoid>(values, right, is_good),
            name + " find_left n=" + std::to_string(size) + " right=" + std::to_string(right));
      }
    }
  }
}

// Sizes where the power-of-two padding changes shape. An off-by-one in the padding survives
// most random sizes and dies here.
void padding_boundaries() {
  testing::section("padding boundaries");
  testing::random_source source(seed ^ 0x1111111111111111ULL);
  const std::vector<std::size_t> sizes = {1,  2,  3,  4,  5,  7,  8,  9,   15,  16,
                                          17, 31, 32, 33, 63, 64, 65, 127, 128, 129};

  for (std::size_t size : sizes) {
    std::vector<std::int64_t> values(size);
    for (std::size_t index = 0; index < size; ++index) {
      values[index] = static_cast<std::int64_t>(source.below(1000));
    }
    ds::sum_segment_tree tree(values);

    check_equal(tree.query(0, size), std::accumulate(values.begin(), values.end(), std::int64_t(0)),
                "full range at n=" + std::to_string(size));
    check_equal(tree.query_all(), tree.query(0, size), "query_all at n=" + std::to_string(size));
    check_equal(tree.query(size, size), std::int64_t(0),
                "empty range at the very end, n=" + std::to_string(size));
    check_equal(tree.query(size - 1, size), values[size - 1],
                "last element alone, n=" + std::to_string(size));
    check_equal(tree.query(0, 1), values[0], "first element alone, n=" + std::to_string(size));
    check_equal(tree.get(size - 1), values[size - 1],
                "get of the last element, n=" + std::to_string(size));

    // Updating the very last real element must not leak into the padding, and updating the
    // first must not be swallowed by it.
    tree.set(size - 1, 777);
    values[size - 1] = 777;
    check_equal(tree.query(0, size), std::accumulate(values.begin(), values.end(), std::int64_t(0)),
                "sum after setting the last element, n=" + std::to_string(size));
    tree.set(0, 555);
    values[0] = 555;
    check_equal(tree.query(0, size), std::accumulate(values.begin(), values.end(), std::int64_t(0)),
                "sum after setting the first element, n=" + std::to_string(size));

    // Padding leaves hold identity, so a min tree must never report one of them.
    ds::min_segment_tree minimums(values);
    check_equal(minimums.query(0, size), *std::min_element(values.begin(), values.end()),
                "min over the full range, n=" + std::to_string(size));
    check_equal(minimums.query_all(), *std::min_element(values.begin(), values.end()),
                "min from the root, n=" + std::to_string(size));
  }
}

void constructors_and_value_semantics() {
  testing::section("constructors and value semantics");

  ds::sum_segment_tree empty_tree;
  check_equal(empty_tree.query_all(), std::int64_t(0), "default constructed folds to identity");
  check_equal(empty_tree.query(0, 0), std::int64_t(0), "default constructed accepts empty range");

  ds::sum_segment_tree sized(5);
  check_equal(sized.query_all(), std::int64_t(0), "size constructor fills with identity");
  for (std::size_t position = 0; position < 5; ++position) {
    check_equal(sized.get(position), std::int64_t(0), "size constructor element is identity");
  }

  ds::min_segment_tree sized_minimums(4);
  check_equal(sized_minimums.query_all(), std::numeric_limits<std::int64_t>::max(),
              "min tree fills with its own identity, not zero");

  ds::max_segment_tree sized_maximums(4);
  check_equal(sized_maximums.query_all(), std::numeric_limits<std::int64_t>::lowest(),
              "max tree fills with its own identity");

  ds::sum_segment_tree zero_sized(0);
  check_equal(zero_sized.query_all(), std::int64_t(0), "explicit size zero folds to identity");
  check_equal(zero_sized.query(0, 0), std::int64_t(0), "explicit size zero accepts empty range");

  // A copy must be independent: mutating one tree must not reach the other.
  ds::sum_segment_tree original(std::vector<std::int64_t>{1, 2, 3, 4});
  ds::sum_segment_tree copy = original;
  copy.set(0, 100);
  check_equal(original.query_all(), std::int64_t(10), "the original is untouched by its copy");
  check_equal(copy.query_all(), std::int64_t(109), "the copy carries its own change");

  ds::sum_segment_tree moved = std::move(copy);
  check_equal(moved.query_all(), std::int64_t(109), "a moved-from tree hands over its contents");
}

void monoid_specific_behaviour() {
  testing::section("monoid specific behaviour");

  std::vector<std::int64_t> values = {5, 1, 9, 3, 7};

  ds::min_segment_tree minimums(values);
  check_equal(minimums.query(1, 4), std::int64_t(1), "min over a middle range");
  minimums.combine_at(1, 4);
  check_equal(minimums.get(1), std::int64_t(1), "combine_at on a min tree never raises a value");
  minimums.combine_at(1, -2);
  check_equal(minimums.get(1), std::int64_t(-2), "combine_at on a min tree lowers a value");
  minimums.set(1, 100);
  check_equal(minimums.get(1), std::int64_t(100), "set on a min tree assigns regardless");
  check_equal(minimums.query_all(), std::int64_t(3), "the root follows a raising set");

  ds::max_segment_tree maximums(values);
  check_equal(maximums.query(0, 3), std::int64_t(9), "max over a prefix");
  maximums.combine_at(0, 2);
  check_equal(maximums.get(0), std::int64_t(5), "combine_at on a max tree never lowers a value");

  ds::gcd_segment_tree divisors(std::vector<std::int64_t>{12, 18, 24});
  check_equal(divisors.query_all(), std::int64_t(6), "gcd of the whole array");
  check_equal(divisors.query(0, 1), std::int64_t(12), "gcd of one element is that element");
  check_equal(divisors.query(1, 1), std::int64_t(0), "gcd of nothing is the identity");

  ds::sum_segment_tree sums(values);
  sums.combine_at(2, 100);
  check_equal(sums.get(2), std::int64_t(109), "combine_at on a sum tree adds");
  check_equal(sums.query_all(), std::int64_t(125), "the root follows combine_at");
}

// Order matters: with a non-commutative monoid, a query that folds its two halves the wrong way
// round produces a reversed string rather than a wrong number, which is unmissable.
void non_commutative_order() {
  testing::section("non-commutative order");

  std::vector<std::string> letters;
  for (std::size_t index = 0; index < 20; ++index) {
    letters.push_back(std::string(1, static_cast<char>('a' + index)));
  }
  algo::data_structures::segment_tree<concat_monoid> tree(letters);

  check_equal(tree.query(0, 20), std::string("abcdefghijklmnopqrst"), "the whole word in order");
  check_equal(tree.query_all(), std::string("abcdefghijklmnopqrst"), "the root holds the order");
  check_equal(tree.query(3, 9), std::string("defghi"), "a middle range keeps its order");
  check_equal(tree.query(19, 20), std::string("t"), "the last letter alone");

  tree.set(5, "F");
  check_equal(tree.query(3, 9), std::string("deFghi"), "an assignment keeps the surrounding order");
  tree.combine_at(0, "!");
  check_equal(tree.query(0, 3), std::string("a!bc"),
              "combine_at appends on the right of the old value");

  std::size_t bound =
      tree.find_right(2, [](const std::string& folded) { return folded.size() <= 4; });
  check_equal(bound, std::size_t(6), "find_right respects the fold, not the element count");
}

void exception_paths() {
  testing::section("exception paths");

  ds::sum_segment_tree tree(std::vector<std::int64_t>{1, 2, 3, 4, 5});
  auto always = [](std::int64_t) { return true; };
  auto never = [](std::int64_t) { return false; };

  check_throws<std::out_of_range>([&] { return tree.get(5); }, "get at n");
  check_throws<std::out_of_range>([&] { return tree.get(6); }, "get past n");
  check_throws<std::out_of_range>([&] { return tree.get(~std::size_t(0)); },
                                  "get of a wrapped-around index");
  check_throws<std::out_of_range>([&] { tree.set(5, 0); }, "set at n");
  check_throws<std::out_of_range>([&] { tree.combine_at(5, 0); }, "combine_at at n");
  check_throws<std::out_of_range>([&] { return tree.query(0, 6); }, "query reaching past n");
  check_throws<std::out_of_range>([&] { return tree.query(6, 6); },
                                  "empty query beyond the end still out of range");
  check_throws<std::out_of_range>([&] { return tree.query(4, 2); }, "reversed query");
  check_throws<std::out_of_range>([&] { return tree.find_right(6, always); }, "find_right past n");
  check_throws<std::out_of_range>([&] { return tree.find_left(6, always); }, "find_left past n");
  check_throws<std::invalid_argument>([&] { return tree.find_right(0, never); },
                                      "find_right with a predicate rejecting identity");
  check_throws<std::invalid_argument>([&] { return tree.find_left(5, never); },
                                      "find_left with a predicate rejecting identity");

  ds::sum_segment_tree empty_tree;
  check_throws<std::out_of_range>([&] { return empty_tree.get(0); }, "get on an empty tree");
  check_throws<std::out_of_range>([&] { return empty_tree.query(0, 1); }, "query on an empty tree");

  // The boundaries that must be accepted. A check that is too eager is as much of a bug as one
  // that is missing.
  check_does_not_throw([&] { return tree.query(5, 5); }, "empty range exactly at the end");
  check_does_not_throw([&] { return tree.query(0, 5); }, "the full range");
  check_does_not_throw([&] { return tree.query(2, 2); }, "an empty range in the middle");
  check_does_not_throw([&] { return tree.get(4); }, "the last valid position");
  check_does_not_throw([&] { tree.set(4, 0); }, "set at the last valid position");
  check_does_not_throw([&] { return tree.find_right(5, always); }, "find_right starting at n");
  check_does_not_throw([&] { return tree.find_left(0, always); }, "find_left ending at zero");
  check_does_not_throw([&] { return empty_tree.query(0, 0); }, "the empty range on an empty tree");

  // A rejected call must leave the structure untouched.
  ds::sum_segment_tree guarded(std::vector<std::int64_t>{1, 2, 3});
  try {
    guarded.set(3, 100);
  } catch (const std::out_of_range&) {
  }
  check_equal(guarded.query_all(), std::int64_t(6), "a rejected set changes nothing");
}

void descent_edge_cases() {
  testing::section("descent edge cases");

  std::vector<std::int64_t> values = {2, 2, 2, 2, 2};
  ds::sum_segment_tree tree(values);
  auto always = [](std::int64_t) { return true; };

  check_equal(tree.find_right(0, always), std::size_t(5),
              "a predicate that never breaks reaches n");
  check_equal(tree.find_left(5, always), std::size_t(0),
              "a predicate that never breaks reaches zero");
  check_equal(tree.find_right(5, always), std::size_t(5), "starting at n returns n");
  check_equal(tree.find_left(0, always), std::size_t(0), "ending at zero returns zero");

  // A predicate that breaks on the very first element: the answer is the empty range.
  auto tiny = [](std::int64_t sum) { return sum < 1; };
  check_equal(tree.find_right(0, tiny), std::size_t(0),
              "breaking immediately yields an empty range");
  check_equal(tree.find_left(5, tiny), std::size_t(5),
              "breaking immediately yields an empty range on the left");

  // Exact boundary: the sum of three elements is exactly the limit, four exceeds it.
  auto exactly_six = [](std::int64_t sum) { return sum <= 6; };
  check_equal(tree.find_right(0, exactly_six), std::size_t(3), "the limit is inclusive");
  check_equal(tree.find_left(5, exactly_six), std::size_t(2), "the limit is inclusive leftwards");

  ds::sum_segment_tree single(std::vector<std::int64_t>{10});
  check_equal(single.find_right(0, [](std::int64_t sum) { return sum <= 9; }), std::size_t(0),
              "a single element that does not fit");
  check_equal(single.find_right(0, [](std::int64_t sum) { return sum <= 10; }), std::size_t(1),
              "a single element that exactly fits");

  ds::sum_segment_tree nothing;
  check_equal(nothing.find_right(0, always), std::size_t(0), "descent on an empty tree");
  check_equal(nothing.find_left(0, always), std::size_t(0), "descent on an empty tree leftwards");
}

// Sizes near 1e3 from a deterministic generator, every answer checked against the obvious
// O(n) scan, which makes the whole section O(n^2).
void large_against_quadratic() {
  testing::section("large, checked against the quadratic reference");
  const std::vector<std::size_t> sizes = {997, 1000, 1024, 1025};

  for (std::size_t size : sizes) {
    testing::random_source source(seed ^ (0x2222222222222222ULL + size));
    std::vector<std::int64_t> values(size);
    for (std::size_t index = 0; index < size; ++index) {
      values[index] = static_cast<std::int64_t>(source.below(2000)) - 1000;
    }

    // Descent needs a monotone predicate, and `sum <= limit` is monotone only while the
    // values cannot pull the sum back down. Signed data therefore gets queried but never
    // descended; a separate non-negative array carries the descent checks.
    std::vector<std::int64_t> magnitudes(size);
    for (std::size_t index = 0; index < size; ++index) {
      magnitudes[index] = static_cast<std::int64_t>(source.below(1000));
    }

    ds::sum_segment_tree sums(values);
    ds::min_segment_tree minimums(values);
    ds::max_segment_tree maximums(values);
    ds::sum_segment_tree ascending(magnitudes);

    for (std::size_t round = 0; round < 400; ++round) {
      std::size_t choice = source.below(4);

      if (choice == 0) {
        std::size_t position = source.below(size);
        std::int64_t value = static_cast<std::int64_t>(source.below(2000)) - 1000;
        sums.set(position, value);
        minimums.set(position, value);
        maximums.set(position, value);
        values[position] = value;
        continue;
      }

      if (choice == 1) {
        std::size_t position = source.below(size);
        std::int64_t value = static_cast<std::int64_t>(source.below(2000)) - 1000;
        sums.combine_at(position, value);
        values[position] += value;
        minimums.set(position, values[position]);
        maximums.set(position, values[position]);
        continue;
      }

      std::size_t left = source.below(size + 1);
      std::size_t right = source.below(size + 1);
      if (left > right) {
        std::size_t swapped = left;
        left = right;
        right = swapped;
      }

      if (choice == 2) {
        std::int64_t expected_sum = 0;
        std::int64_t expected_min = std::numeric_limits<std::int64_t>::max();
        std::int64_t expected_max = std::numeric_limits<std::int64_t>::lowest();
        for (std::size_t index = left; index < right; ++index) {
          expected_sum += values[index];
          expected_min = std::min(expected_min, values[index]);
          expected_max = std::max(expected_max, values[index]);
        }
        check_equal(sums.query(left, right), expected_sum, "sum " + describe(size, left, right));
        check_equal(minimums.query(left, right), expected_min,
                    "min " + describe(size, left, right));
        check_equal(maximums.query(left, right), expected_max,
                    "max " + describe(size, left, right));
        continue;
      }

      // Descent against the same linear scan the small layer uses.
      std::int64_t limit = static_cast<std::int64_t>(source.below(200000));
      auto is_good = [limit](std::int64_t folded) { return folded <= limit; };
      check_equal(ascending.find_right(left, is_good),
                  linear_find_right<ds::sum_monoid<std::int64_t>>(magnitudes, left, is_good),
                  "find_right n=" + std::to_string(size) + " left=" + std::to_string(left));
      check_equal(ascending.find_left(right, is_good),
                  linear_find_left<ds::sum_monoid<std::int64_t>>(magnitudes, right, is_good),
                  "find_left n=" + std::to_string(size) + " right=" + std::to_string(right));
    }

    check_equal(sums.query_all(), std::accumulate(values.begin(), values.end(), std::int64_t(0)),
                "the root after the whole run, n=" + std::to_string(size));
  }
}

}  // namespace

int main() {
  exhaustive_small<ds::sum_monoid<std::int64_t>>(
      "sum",
      [](testing::random_source& source) { return static_cast<std::int64_t>(source.below(100)); },
      [](testing::random_source& source) {
        std::int64_t limit = static_cast<std::int64_t>(source.below(400));
        return [limit](std::int64_t folded) { return folded <= limit; };
      });

  exhaustive_small<ds::min_monoid<std::int64_t>>(
      "min",
      [](testing::random_source& source) { return static_cast<std::int64_t>(source.below(50)); },
      [](testing::random_source& source) {
        std::int64_t bound = static_cast<std::int64_t>(source.below(50));
        return [bound](std::int64_t folded) { return folded >= bound; };
      });

  exhaustive_small<ds::max_monoid<std::int64_t>>(
      "max",
      [](testing::random_source& source) { return static_cast<std::int64_t>(source.below(50)); },
      [](testing::random_source& source) {
        std::int64_t bound = static_cast<std::int64_t>(source.below(50));
        return [bound](std::int64_t folded) { return folded <= bound; };
      });

  exhaustive_small<ds::gcd_monoid<std::int64_t>>(
      "gcd",
      [](testing::random_source& source) {
        return static_cast<std::int64_t>(1 + source.below(60));
      },
      [](testing::random_source& source) {
        std::int64_t divisor = static_cast<std::int64_t>(1 + source.below(4));
        return [divisor](std::int64_t folded) { return folded % divisor == 0; };
      });

  exhaustive_small<concat_monoid>(
      "concat",
      [](testing::random_source& source) {
        return std::string(1, static_cast<char>('a' + source.below(4)));
      },
      [](testing::random_source& source) {
        std::size_t limit = source.below(8);
        return [limit](const std::string& folded) { return folded.size() <= limit; };
      });

  padding_boundaries();
  constructors_and_value_semantics();
  monoid_specific_behaviour();
  non_commutative_order();
  exception_paths();
  descent_edge_cases();
  large_against_quadratic();

  return testing::summarize("segment_tree") == 0 ? 0 : 1;
}
