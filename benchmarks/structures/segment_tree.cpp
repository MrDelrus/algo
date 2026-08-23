// Timing gate for algo::data_structures::segment_tree.
//
// Not a report. This runs in CI and fails the build when the structure stops being fast
// enough, so its job is a verdict, not a number to publish. The number is printed anyway,
// because a budget that only says pass hides a slow slide from 15 ms towards the limit until
// the day it crosses it.
//
// Fully deterministic: the operation stream comes from a counter-based splitmix64 with a fixed
// seed, so every run performs exactly the same work in exactly the same order. No std
// distribution is used, because those are not specified to produce identical output across
// standard library implementations.
//
// The budget is deliberately loose. A shared CI runner is slower and noisier than a desktop,
// while the regression this guards against — an O(log n) operation quietly becoming O(n) —
// overshoots any budget by orders of magnitude rather than by percent.

#include "algo_library.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr std::size_t tree_size = 200000;
constexpr std::size_t operation_count = 200000;
constexpr std::size_t repetition_count = 5;
constexpr double budget_milliseconds = 100.0;
constexpr std::uint64_t seed = 0x243f6a8885a308d3ULL;

enum operation_kind : std::size_t {
  operation_set = 0,
  operation_combine_at = 1,
  operation_get = 2,
  operation_query = 3,
  operation_query_all = 4,
  operation_kind_count = 5
};

class random_source {
 public:
  explicit random_source(std::uint64_t initial_state) : _state(initial_state) {}

  std::uint64_t next() {
    _state += 0x9e3779b97f4a7c15ULL;
    std::uint64_t mixed = _state;
    mixed = (mixed ^ (mixed >> 30)) * 0xbf58476d1ce4e5b9ULL;
    mixed = (mixed ^ (mixed >> 27)) * 0x94d049bb133111ebULL;
    return mixed ^ (mixed >> 31);
  }

  std::size_t below(std::size_t bound) {
    return static_cast<std::size_t>(next() % bound);
  }

  std::int64_t value() {
    return static_cast<std::int64_t>(next() % 1000000000ULL);
  }

 private:
  std::uint64_t _state;
};

struct scenario {
  std::string title;
  // Shares out of ten, indexed by operation_kind. They must sum to ten.
  std::array<std::size_t, operation_kind_count> shares;
  // When true, every point operation targets one position drawn once.
  bool single_position;
};

std::vector<std::int64_t> make_initial_values(std::size_t size) {
  random_source source(seed ^ 0x5555555555555555ULL);
  std::vector<std::int64_t> values(size);
  for (std::size_t index = 0; index < size; ++index) {
    values[index] = source.value();
  }
  return values;
}

std::uint64_t replay(ds::sum_segment_tree& tree, const scenario& plan, std::size_t size,
                     std::size_t operations) {
  random_source source(seed);
  std::size_t pinned_position = plan.single_position ? source.below(size) : 0;
  std::uint64_t checksum = 0;

  for (std::size_t step = 0; step < operations; ++step) {
    std::size_t choice = static_cast<std::size_t>(source.next() % 10);
    std::size_t kind = 0;
    std::size_t threshold = plan.shares[0];
    while (choice >= threshold && kind + 1 < operation_kind_count) {
      ++kind;
      threshold += plan.shares[kind];
    }

    if (kind == operation_query) {
      std::size_t first = source.below(size + 1);
      std::size_t second = source.below(size + 1);
      if (first > second) {
        std::size_t swapped = first;
        first = second;
        second = swapped;
      }
      checksum += static_cast<std::uint64_t>(tree.query(first, second));
      continue;
    }

    if (kind == operation_query_all) {
      checksum += static_cast<std::uint64_t>(tree.query_all());
      continue;
    }

    std::size_t position = plan.single_position ? pinned_position : source.below(size);
    if (kind == operation_get) {
      checksum += static_cast<std::uint64_t>(tree.get(position));
      continue;
    }

    std::int64_t value = source.value();
    if (kind == operation_set) {
      tree.set(position, value);
    } else {
      tree.combine_at(position, value);
    }
  }

  return checksum;
}

// The fastest repetition is the verdict. Every repetition does bitwise identical work, so the
// differences between them are interference from outside the process, and holding the build to
// the unluckiest run would make the gate flaky rather than strict.
double measure(const scenario& plan, const std::vector<std::int64_t>& initial, std::size_t size,
               std::size_t operations, std::uint64_t& checksum) {
  std::vector<double> timings;
  for (std::size_t repetition = 0; repetition < repetition_count; ++repetition) {
    ds::sum_segment_tree tree(initial);
    auto started = std::chrono::steady_clock::now();
    checksum = replay(tree, plan, size, operations);
    auto finished = std::chrono::steady_clock::now();
    timings.push_back(std::chrono::duration<double, std::milli>(finished - started).count());
  }
  std::sort(timings.begin(), timings.end());
  return timings.front();
}

}  // namespace

int main() {
  const std::vector<scenario> scenarios = {
      {"one hot position", {5, 0, 5, 0, 0}, true},
      {"balanced", {2, 2, 2, 3, 1}, false},
      {"query heavy", {1, 1, 1, 5, 2}, false},
  };

  const std::vector<std::int64_t> warmup_values = make_initial_values(1000);
  std::uint64_t discarded = 0;
  for (const scenario& plan : scenarios) {
    measure(plan, warmup_values, 1000, 1000, discarded);
  }

  const std::vector<std::int64_t> values = make_initial_values(tree_size);

  std::cout << "ds::sum_segment_tree   n = " << tree_size << ", operations = " << operation_count
            << ", best of " << repetition_count << " runs, budget " << budget_milliseconds
            << " ms\n\n";

  bool over_budget = false;
  for (const scenario& plan : scenarios) {
    std::uint64_t checksum = 0;
    double elapsed = measure(plan, values, tree_size, operation_count, checksum);
    bool failed = elapsed > budget_milliseconds;
    over_budget = over_budget || failed;
    std::cout << std::left << std::setw(20) << plan.title << std::right << std::fixed
              << std::setprecision(2) << std::setw(9) << elapsed << " ms" << std::setw(8)
              << std::setprecision(0) << 100.0 * elapsed / budget_milliseconds << "% of budget   "
              << (failed ? "OVER BUDGET" : "ok") << "   checksum " << checksum << '\n';
  }

  if (over_budget) {
    std::cout << "\nFAILED: a scenario exceeded " << budget_milliseconds << " ms\n";
    return 1;
  }
  std::cout << "\nall scenarios within budget\n";
  return 0;
}
