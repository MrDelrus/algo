// Speed benchmark for algo::data_structures::segment_tree.
//
// Measures throughput only — correctness lives in tests, not here. Fully deterministic: the
// operation stream comes from a counter-based splitmix64 with a fixed seed, so every run of
// this binary performs exactly the same work in exactly the same order. No std distribution is
// used anywhere, because std distributions are not specified to produce identical output
// across standard library implementations.
//
// The stream is not precomputed: 2e7 operations would cost hundreds of megabytes, well past
// what a judge grants. It is generated inside the measured loop instead, and the generator's
// own cost is measured separately so it can be subtracted rather than silently attributed to
// the structure.
//
// Per-operation timings come from replaying the identical stream with a mask that enables one
// operation kind at a time. Every pass draws the same numbers in the same order, so the only
// difference between two passes is the tree work they perform. Subtracting the pass with an
// empty mask leaves the cost of that operation alone.
//
// Build and run through benchmarks/run.sh, which pins resources in a container.

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
constexpr std::size_t operation_count = 20000000;
constexpr std::size_t repetition_count = 5;
constexpr std::size_t warmup_tree_size = 10000;
constexpr std::size_t warmup_operation_count = 100000;
constexpr std::uint64_t seed = 0x243f6a8885a308d3ULL;

enum operation_kind : std::size_t {
  operation_set = 0,
  operation_combine_at = 1,
  operation_get = 2,
  operation_query = 3,
  operation_query_all = 4,
  operation_kind_count = 5
};

const std::array<std::string, operation_kind_count> operation_names = {"set", "combine_at", "get",
                                                                       "query", "query_all"};

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
  std::string description;
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

// Replays the scenario's stream. Random numbers are always drawn, whatever the mask says, so
// every mask walks the identical stream; the mask only decides which calls reach the tree.
std::uint64_t replay(ds::segment_tree<ds::sum_monoid<std::int64_t>>& tree, const scenario& plan,
                     std::uint64_t enabled, std::size_t size, std::size_t operations,
                     std::array<std::size_t, operation_kind_count>& counts) {
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
    ++counts[kind];

    bool execute = ((enabled >> kind) & 1) != 0;

    if (kind == operation_query) {
      std::size_t first = source.below(size + 1);
      std::size_t second = source.below(size + 1);
      if (first > second) {
        std::size_t swapped = first;
        first = second;
        second = swapped;
      }
      if (execute) {
        checksum += static_cast<std::uint64_t>(tree.query(first, second));
      }
      continue;
    }

    if (kind == operation_query_all) {
      if (execute) {
        checksum += static_cast<std::uint64_t>(tree.query_all());
      }
      continue;
    }

    std::size_t position = plan.single_position ? pinned_position : source.below(size);
    if (kind == operation_get) {
      if (execute) {
        checksum += static_cast<std::uint64_t>(tree.get(position));
      }
      continue;
    }

    std::int64_t value = source.value();
    if (execute) {
      if (kind == operation_set) {
        tree.set(position, value);
      } else {
        tree.combine_at(position, value);
      }
    }
  }

  return checksum;
}

struct timing {
  double best_milliseconds = 0.0;
  double worst_milliseconds = 0.0;
  std::uint64_t checksum = 0;
  std::array<std::size_t, operation_kind_count> counts = {};
};

// The reported figure is the fastest run. Every repetition performs bitwise identical work, so
// the differences between them come from outside the process, and the fastest run is the one
// that suffered the least interference. The spread between fastest and slowest is printed too,
// so a noisy machine cannot pass itself off as a clean measurement.
timing measure(const scenario& plan, std::uint64_t enabled,
               const std::vector<std::int64_t>& initial, std::size_t size, std::size_t operations) {
  timing result;
  std::vector<double> timings;
  for (std::size_t repetition = 0; repetition < repetition_count; ++repetition) {
    ds::segment_tree<ds::sum_monoid<std::int64_t>> tree(initial);
    std::array<std::size_t, operation_kind_count> counts = {};
    auto started = std::chrono::steady_clock::now();
    result.checksum = replay(tree, plan, enabled, size, operations, counts);
    auto finished = std::chrono::steady_clock::now();
    timings.push_back(std::chrono::duration<double, std::milli>(finished - started).count());
    result.counts = counts;
  }
  std::sort(timings.begin(), timings.end());
  result.best_milliseconds = timings.front();
  result.worst_milliseconds = timings.back();
  return result;
}

double measure_build(const std::vector<std::int64_t>& initial) {
  std::vector<double> timings;
  std::uint64_t sink = 0;
  for (std::size_t repetition = 0; repetition < repetition_count; ++repetition) {
    auto started = std::chrono::steady_clock::now();
    ds::segment_tree<ds::sum_monoid<std::int64_t>> tree(initial);
    auto finished = std::chrono::steady_clock::now();
    sink += static_cast<std::uint64_t>(tree.query_all());
    timings.push_back(std::chrono::duration<double, std::milli>(finished - started).count());
  }
  std::sort(timings.begin(), timings.end());
  if (sink == 1) {
    std::cout << "";  // keeps the build from being optimised away
  }
  return timings.front();
}

void run_scenario(const scenario& plan, const std::vector<std::int64_t>& initial) {
  std::cout << "\n" << plan.title << "\n" << plan.description << "\n\n";

  timing empty = measure(plan, 0, initial, tree_size, operation_count);
  timing full = measure(plan, ~std::uint64_t(0), initial, tree_size, operation_count);

  std::cout << std::left << std::setw(14) << "total" << std::right << std::fixed
            << std::setprecision(2) << std::setw(10) << full.best_milliseconds << " ms"
            << std::setw(9) << std::setprecision(1)
            << full.best_milliseconds * 1000000.0 / static_cast<double>(operation_count) << " ns/op"
            << std::setw(8) << std::setprecision(1)
            << 100.0 * (full.worst_milliseconds - full.best_milliseconds) / full.best_milliseconds
            << "% spread\n";
  std::cout << std::left << std::setw(14) << "generator" << std::right << std::setw(10)
            << std::setprecision(2) << empty.best_milliseconds << " ms" << std::setw(9)
            << std::setprecision(1)
            << empty.best_milliseconds * 1000000.0 / static_cast<double>(operation_count)
            << " ns/op\n\n";

  std::cout << std::left << std::setw(14) << "operation" << std::right << std::setw(12) << "calls"
            << std::setw(13) << "ns per call" << '\n';
  for (std::size_t kind = 0; kind < operation_kind_count; ++kind) {
    if (full.counts[kind] == 0) {
      continue;
    }
    timing isolated = measure(plan, std::uint64_t(1) << kind, initial, tree_size, operation_count);
    double nanoseconds = (isolated.best_milliseconds - empty.best_milliseconds) * 1000000.0 /
                         static_cast<double>(full.counts[kind]);
    std::cout << std::left << std::setw(14) << operation_names[kind] << std::right << std::setw(12)
              << full.counts[kind] << std::setw(13) << std::setprecision(1) << nanoseconds << '\n';
  }
  std::cout << "\nchecksum " << full.checksum << '\n';
}

}  // namespace

int main() {
  std::vector<scenario> scenarios = {
      {"scenario 1 — one hot position",
       "Half get, half set, every one of them aimed at a single position drawn once.\n"
       "The root-to-leaf path stays in cache, so this isolates the cost of walking the tree\n"
       "with memory latency taken out of the picture.",
       {5, 0, 5, 0, 0},
       true},
      {"scenario 2 — balanced",
       "Random positions. set 2/10, combine_at 2/10, get 2/10, query 3/10, query_all 1/10.\n"
       "Point updates and range queries carry comparable weight, and every access misses cache.",
       {2, 2, 2, 3, 1},
       false},
      {"scenario 3 — query heavy",
       "Random positions. set 1/10, combine_at 1/10, get 1/10, query 5/10, query_all 2/10.\n"
       "Weight moves onto range queries, so the difference from scenario 2 is the price of\n"
       "query relative to a point update.",
       {1, 1, 1, 5, 2},
       false},
  };

  std::vector<std::int64_t> warmup_values = make_initial_values(warmup_tree_size);
  for (const scenario& plan : scenarios) {
    measure(plan, ~std::uint64_t(0), warmup_values, warmup_tree_size, warmup_operation_count);
  }

  std::vector<std::int64_t> values = make_initial_values(tree_size);

  std::cout << "segment_tree<sum_monoid<int64_t>>\n"
            << "n = " << tree_size << ", operations = " << operation_count << ", best of "
            << repetition_count << " runs\n\n"
            << std::left << std::setw(14) << "build" << std::right << std::fixed
            << std::setprecision(2) << std::setw(10) << measure_build(values) << " ms\n";

  for (const scenario& plan : scenarios) {
    run_scenario(plan, values);
  }

  return 0;
}
