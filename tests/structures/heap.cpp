// Correctness tests for algo::data_structures::heap_min, heap_max and heapify.
//
// The reference is a sorted copy of the input: a heap must hand its elements back in that order,
// ascending for heap_min and descending for heap_max. Deterministic throughout.

#include "algo_library.hpp"

#include "../harness.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace {

using testing::check;
using testing::check_equal;

constexpr std::uint64_t seed = 0x9e3779b97f4a7c15ULL;

std::vector<std::int64_t> generate(std::size_t count, std::int64_t bound) {
  testing::random_source source(seed ^ count);
  std::vector<std::int64_t> values(count);
  for (std::size_t index = 0; index < count; ++index) {
    values[index] =
        static_cast<std::int64_t>(source.below(static_cast<std::size_t>(bound))) - bound / 2;
  }
  return values;
}

// Drains a heap and checks it came out in the order the reference says.
template <typename heap_type>
void check_order(heap_type heap, std::vector<std::int64_t> expected, const std::string& note) {
  check_equal(heap.size(), expected.size(), note + ": size before draining");
  for (std::size_t index = 0; index < expected.size(); ++index) {
    check_equal(heap.top(), expected[index], note + ": element " + std::to_string(index));
    heap.pop();
  }
  check(heap.empty(), note + ": empty after draining");
}

void pushed_one_at_a_time() {
  testing::section("pushed one at a time");

  for (std::size_t count : {std::size_t(0), std::size_t(1), std::size_t(2), std::size_t(7),
                            std::size_t(64), std::size_t(1000)}) {
    std::vector<std::int64_t> values = generate(count, 500);

    ds::heap_min<std::int64_t> smallest;
    ds::heap_max<std::int64_t> largest;
    for (std::int64_t value : values) {
      smallest.push(value);
      largest.push(value);
    }

    std::vector<std::int64_t> ascending = values;
    std::sort(ascending.begin(), ascending.end());
    std::vector<std::int64_t> descending = ascending;
    std::reverse(descending.begin(), descending.end());

    check_order(smallest, ascending, "heap_min of " + std::to_string(count));
    check_order(largest, descending, "heap_max of " + std::to_string(count));
  }
}

void built_by_heapify() {
  testing::section("built by heapify");

  for (std::size_t count : {std::size_t(0), std::size_t(1), std::size_t(5), std::size_t(200)}) {
    std::vector<std::int64_t> values = generate(count, 300);
    std::vector<std::int64_t> ascending = values;
    std::sort(ascending.begin(), ascending.end());
    std::vector<std::int64_t> descending = ascending;
    std::reverse(descending.begin(), descending.end());

    check_order(ds::heapify(values), ascending,
                "heapify defaults to heap_min, n=" + std::to_string(count));
    check_order(ds::heapify<ds::heap_max>(values), descending,
                "heapify<heap_max>, n=" + std::to_string(count));

    // Building in one go must agree with pushing one at a time.
    ds::heap_min<std::int64_t> pushed;
    for (std::int64_t value : values) {
      pushed.push(value);
    }
    check_order(pushed, ascending, "pushing matches heapify, n=" + std::to_string(count));
  }
}

void heapify_takes_ownership() {
  testing::section("heapify takes ownership");

  std::vector<std::int64_t> values = generate(50, 200);
  std::vector<std::int64_t> ascending = values;
  std::sort(ascending.begin(), ascending.end());

  std::vector<std::int64_t> donated = values;
  auto heap = ds::heapify(std::move(donated));
  check_order(heap, ascending, "a heap built from a moved vector holds the same elements");

  std::vector<std::int64_t> copied = values;
  auto from_copy = ds::heapify(copied);
  check_equal(copied.size(), values.size(), "the const overload leaves its argument alone");
  check_order(from_copy, ascending, "and produces the same heap");
}

void duplicates_and_extremes() {
  testing::section("duplicates and extremes");

  std::vector<std::int64_t> repeated(100, 42);
  std::vector<std::int64_t> expected(100, 42);
  check_order(ds::heapify(repeated), expected, "a heap of equal elements");

  const std::int64_t low = std::numeric_limits<std::int64_t>::min();
  const std::int64_t high = std::numeric_limits<std::int64_t>::max();
  std::vector<std::int64_t> extremes = {high, low, 0, high, low};
  std::vector<std::int64_t> ascending = extremes;
  std::sort(ascending.begin(), ascending.end());
  std::vector<std::int64_t> descending = ascending;
  std::reverse(descending.begin(), descending.end());
  check_order(ds::heapify(extremes), ascending, "the extremes of the range, smallest first");
  check_order(ds::heapify<ds::heap_max>(extremes), descending, "and largest first");
}

// Pairs are what Dijkstra will push, and they order lexicographically.
void pairs_order_by_first_then_second() {
  testing::section("pairs order by first then second");

  std::vector<std::pair<std::int64_t, std::int64_t>> entries = {
      {7, 100}, {2, 300}, {7, 5}, {1, 999}, {2, 1}};
  ds::heap_min<std::pair<std::int64_t, std::int64_t>> queue;
  for (const auto& entry : entries) {
    queue.push(entry);
  }

  std::vector<std::pair<std::int64_t, std::int64_t>> expected = entries;
  std::sort(expected.begin(), expected.end());
  for (const auto& want : expected) {
    check(queue.top() == want, "pairs come out in lexicographic order");
    queue.pop();
  }
  check(queue.empty(), "and the queue empties");
}

}  // namespace

int main() {
  pushed_one_at_a_time();
  built_by_heapify();
  heapify_takes_ownership();
  duplicates_and_extremes();
  pairs_order_by_first_then_second();

  return testing::summarize("heap") == 0 ? 0 : 1;
}
