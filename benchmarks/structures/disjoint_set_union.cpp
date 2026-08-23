// Timing gate for algo::data_structures::disjoint_set_union.
//
// Not a report. This runs in CI and fails the build when the structure stops being fast enough.
// The measured time is printed anyway, because a gate that only says pass hides a slow slide
// toward the limit until the day it crosses.
//
// Fully deterministic: the operation stream comes from a counter-based splitmix64 with a fixed
// seed. No std distribution is used, because those are not specified to produce identical
// output across standard library implementations.
//
// What the scenarios are shaped to catch: losing either heuristic. Without union by size or
// without path compression a merge chain degrades to O(log n) per operation, and without both
// it degrades to O(n) — the sequential scenario builds exactly the long path that punishes it.

#include "algo_library.hpp"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr std::size_t vertex_count = 200000;
constexpr std::size_t operation_count = 200000;
constexpr std::size_t repetition_count = 5;
constexpr double budget_milliseconds = 100.0;
constexpr std::uint64_t seed = 0x452821e638d01377ULL;

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

 private:
  std::uint64_t _state;
};

// Random merges and queries in balance. Components grow into a few large ones, so most merges
// stop joining anything and the work becomes finding roots.
std::uint64_t scenario_mixed(std::size_t size, std::size_t operations) {
  random_source source(seed);
  ds::disjoint_set_union structure(size);
  std::uint64_t checksum = 0;
  for (std::size_t step = 0; step < operations; ++step) {
    std::size_t first = source.below(size);
    std::size_t second = source.below(size);
    if (source.next() % 2 == 0) {
      checksum += structure.merge(first, second) ? 1 : 0;
    } else {
      checksum += structure.is_connected(first, second) ? 1 : 0;
      checksum += structure.get_component_size(first);
    }
  }
  return checksum + structure.get_component_count();
}

// Merge everything into one component in order, then query across it. Sequential merging is the
// shape that builds a long path when the heuristics are missing.
std::uint64_t scenario_sequential(std::size_t size, std::size_t operations) {
  random_source source(seed);
  ds::disjoint_set_union structure(size);
  std::uint64_t checksum = 0;
  for (std::size_t vertex = 0; vertex + 1 < size; ++vertex) {
    checksum += structure.merge(vertex, vertex + 1) ? 1 : 0;
  }
  for (std::size_t step = 0; step < operations; ++step) {
    checksum += structure.get_ancestor(source.below(size));
  }
  return checksum;
}

// Merge in a pattern that pairs distant vertices, so components meet as equals and union by
// size has to choose a root on every step.
std::uint64_t scenario_pairwise(std::size_t size, std::size_t operations) {
  random_source source(seed);
  ds::disjoint_set_union structure(size);
  std::uint64_t checksum = 0;
  for (std::size_t stride = 1; stride < size; stride *= 2) {
    for (std::size_t vertex = 0; vertex + stride < size; vertex += 2 * stride) {
      checksum += structure.merge(vertex, vertex + stride) ? 1 : 0;
    }
  }
  for (std::size_t step = 0; step < operations; ++step) {
    checksum += structure.is_connected(source.below(size), source.below(size)) ? 1 : 0;
  }
  return checksum;
}

// The fastest repetition is the verdict. Every repetition does bitwise identical work, so the
// differences between them are interference from outside the process, and holding the build to
// the unluckiest run would make the gate flaky rather than strict.
template <typename scenario>
double measure(scenario run, std::size_t size, std::size_t operations, std::uint64_t& checksum) {
  std::vector<double> timings;
  for (std::size_t repetition = 0; repetition < repetition_count; ++repetition) {
    auto started = std::chrono::steady_clock::now();
    checksum = run(size, operations);
    auto finished = std::chrono::steady_clock::now();
    timings.push_back(std::chrono::duration<double, std::milli>(finished - started).count());
  }
  std::sort(timings.begin(), timings.end());
  return timings.front();
}

}  // namespace

int main() {
  std::uint64_t discarded = 0;
  measure(scenario_mixed, 1000, 1000, discarded);
  measure(scenario_sequential, 1000, 1000, discarded);
  measure(scenario_pairwise, 1000, 1000, discarded);

  std::cout << "ds::disjoint_set_union   n = " << vertex_count
            << ", operations = " << operation_count << ", best of " << repetition_count
            << " runs, budget " << budget_milliseconds << " ms\n\n";

  bool over_budget = false;
  const std::vector<std::pair<std::string, std::uint64_t (*)(std::size_t, std::size_t)>> scenarios =
      {{"mixed", scenario_mixed},
       {"sequential merges", scenario_sequential},
       {"pairwise merges", scenario_pairwise}};

  for (const auto& entry : scenarios) {
    std::uint64_t checksum = 0;
    double elapsed = measure(entry.second, vertex_count, operation_count, checksum);
    bool failed = elapsed > budget_milliseconds;
    over_budget = over_budget || failed;
    std::cout << std::left << std::setw(20) << entry.first << std::right << std::fixed
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
