// Correctness tests for algo::data_structures::disjoint_set_union.
//
// The reference is a plain array of labels: merging relabels every vertex of one component in
// O(n), answering is a comparison. Quadratic and obviously right, which is the whole point —
// the structure under test is the clever one, the reference must not be.
//
// The small layer is exhaustive over n up to 32 and checks the full pairwise relation after
// every single merge, so a component that splits or leaks into another is caught the moment it
// happens rather than at the end. The large layer runs sizes near 1e3 against the same
// reference, which makes it quadratic overall.
//
// Everything is deterministic: fixed-seed counter-based splitmix64, no std distribution.

#include "algo_library.hpp"

#include "../harness.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {

using testing::check;
using testing::check_equal;

constexpr std::size_t exhaustive_limit = 32;
constexpr std::uint64_t seed = 0x517cc1b727220a95ULL;

// The obvious reference: one label per vertex, merging rewrites labels the slow way.
class labelled_partition {
 public:
  explicit labelled_partition(std::size_t size) : _label(size) {
    for (std::size_t vertex = 0; vertex < size; ++vertex) {
      _label[vertex] = vertex;
    }
  }

  bool merge(std::size_t first, std::size_t second) {
    std::size_t from = _label[second];
    std::size_t to = _label[first];
    if (from == to) {
      return false;
    }
    for (std::size_t vertex = 0; vertex < _label.size(); ++vertex) {
      if (_label[vertex] == from) {
        _label[vertex] = to;
      }
    }
    return true;
  }

  bool is_connected(std::size_t first, std::size_t second) const {
    return _label[first] == _label[second];
  }

  std::size_t component_size(std::size_t vertex) const {
    std::size_t count = 0;
    for (std::size_t other = 0; other < _label.size(); ++other) {
      count += _label[other] == _label[vertex] ? 1 : 0;
    }
    return count;
  }

  std::size_t component_count() const {
    std::size_t count = 0;
    for (std::size_t vertex = 0; vertex < _label.size(); ++vertex) {
      count += _label[vertex] == vertex ? 1 : 0;
    }
    return count;
  }

 private:
  std::vector<std::size_t> _label;
};

std::string at(std::size_t size, std::size_t first, std::size_t second) {
  return "n=" + std::to_string(size) + " (" + std::to_string(first) + ", " +
         std::to_string(second) + ")";
}

// Every pair, every size, every count — after each merge.
void check_against_reference(const ds::disjoint_set_union& structure,
                             const labelled_partition& reference, std::size_t size,
                             const std::string& note) {
  for (std::size_t first = 0; first < size; ++first) {
    for (std::size_t second = 0; second < size; ++second) {
      check(structure.is_connected(first, second) == reference.is_connected(first, second),
            note + " is_connected " + at(size, first, second));
    }
  }
  for (std::size_t vertex = 0; vertex < size; ++vertex) {
    check_equal(
        structure.get_component_size(vertex), reference.component_size(vertex),
        note + " get_component_size at " + std::to_string(vertex) + ", n=" + std::to_string(size));
  }
  check_equal(structure.get_component_count(), reference.component_count(),
              note + " get_component_count, n=" + std::to_string(size));
}

void exhaustive_small() {
  testing::section("exhaustive n <= " + std::to_string(exhaustive_limit));
  testing::random_source source(seed);

  for (std::size_t size = 1; size <= exhaustive_limit; ++size) {
    ds::disjoint_set_union structure(size);
    labelled_partition reference(size);
    check_against_reference(structure, reference, size, "fresh");

    // Enough merges to collapse everything and then keep merging what is already merged, so the
    // "already together" path is exercised as heavily as the joining one.
    for (std::size_t round = 0; round < 3 * size; ++round) {
      std::size_t first = source.below(size);
      std::size_t second = source.below(size);
      check_equal(structure.merge(first, second), reference.merge(first, second),
                  "merge returns whether it joined, " + at(size, first, second));
      check_against_reference(structure, reference, size, "after merge");
    }
  }
}

// An ancestor is not specified to be any particular vertex, but it must be stable while nothing
// merges, shared by exactly the vertices of one component, and a member of that component.
void ancestor_contract() {
  testing::section("ancestor contract");
  testing::random_source source(seed ^ 0x3333333333333333ULL);

  const std::size_t size = 64;
  ds::disjoint_set_union structure(size);
  for (std::size_t round = 0; round < 40; ++round) {
    structure.merge(source.below(size), source.below(size));
  }

  for (std::size_t vertex = 0; vertex < size; ++vertex) {
    std::size_t ancestor = structure.get_ancestor(vertex);
    check(ancestor < size, "the ancestor is a real vertex");
    check(structure.is_connected(vertex, ancestor), "the ancestor lies in the same component");
    check_equal(structure.get_ancestor(vertex), ancestor, "asking twice gives the same answer");
    check_equal(structure.get_ancestor(ancestor), ancestor, "an ancestor is its own ancestor");
  }

  for (std::size_t first = 0; first < size; ++first) {
    for (std::size_t second = 0; second < size; ++second) {
      bool same_ancestor = structure.get_ancestor(first) == structure.get_ancestor(second);
      check_equal(same_ancestor, structure.is_connected(first, second),
                  "a shared ancestor means exactly connected, " + at(size, first, second));
    }
  }

  // Compression must not disturb anything: read every path to the root, then re-verify.
  ds::disjoint_set_union before(size);
  ds::disjoint_set_union after(size);
  testing::random_source replay(seed ^ 0x4444444444444444ULL);
  for (std::size_t round = 0; round < 40; ++round) {
    std::size_t first = replay.below(size);
    std::size_t second = replay.below(size);
    before.merge(first, second);
    after.merge(first, second);
  }
  for (std::size_t vertex = 0; vertex < size; ++vertex) {
    after.get_ancestor(vertex);
  }
  for (std::size_t first = 0; first < size; ++first) {
    for (std::size_t second = 0; second < size; ++second) {
      check_equal(after.is_connected(first, second), before.is_connected(first, second),
                  "compression leaves the partition alone, " + at(size, first, second));
    }
  }
}

void shapes_that_stress_the_path() {
  testing::section("shapes that stress the path");

  // A chain merged in order: each merge attaches a singleton to a growing component, which is
  // the case union by size is supposed to keep shallow.
  const std::size_t size = 1000;
  ds::disjoint_set_union chain(size);
  for (std::size_t vertex = 0; vertex + 1 < size; ++vertex) {
    check(chain.merge(vertex, vertex + 1), "each link of a chain joins something new");
  }
  check_equal(chain.get_component_count(), std::size_t(1), "a full chain is one component");
  check_equal(chain.get_component_size(0), size, "the chain component holds everything");
  check(chain.is_connected(0, size - 1), "the ends of a chain are connected");
  check(!chain.merge(0, size - 1), "merging inside one component joins nothing");

  // The same chain built backwards, and one built from the middle outwards.
  ds::disjoint_set_union backwards(size);
  for (std::size_t vertex = size - 1; vertex > 0; --vertex) {
    backwards.merge(vertex, vertex - 1);
  }
  check_equal(backwards.get_component_count(), std::size_t(1), "backwards chain collapses too");

  // Two halves joined last: the moment a merge picks the larger root matters here.
  ds::disjoint_set_union halves(size);
  for (std::size_t vertex = 0; vertex + 1 < size / 2; ++vertex) {
    halves.merge(vertex, vertex + 1);
  }
  for (std::size_t vertex = size / 2; vertex + 1 < size; ++vertex) {
    halves.merge(vertex, vertex + 1);
  }
  check_equal(halves.get_component_count(), std::size_t(2), "two halves are two components");
  check_equal(halves.get_component_size(0), size / 2, "the first half is half the size");
  check(!halves.is_connected(0, size - 1), "the halves are not connected yet");
  check(halves.merge(0, size - 1), "joining the halves joins something");
  check_equal(halves.get_component_count(), std::size_t(1), "and leaves one component");
}

void singletons_and_self_merges() {
  testing::section("singletons and self merges");

  ds::disjoint_set_union alone(1);
  check_equal(alone.get_component_count(), std::size_t(1), "one vertex is one component");
  check_equal(alone.get_component_size(0), std::size_t(1), "of size one");
  check(alone.is_connected(0, 0), "a vertex is connected to itself");
  check(!alone.merge(0, 0), "merging a vertex with itself joins nothing");
  check_equal(alone.get_component_count(), std::size_t(1), "and changes no count");

  ds::disjoint_set_union untouched(5);
  check_equal(untouched.get_component_count(), std::size_t(5), "nothing merged, n components");
  for (std::size_t vertex = 0; vertex < 5; ++vertex) {
    check_equal(untouched.get_component_size(vertex), std::size_t(1), "each alone");
    check_equal(untouched.get_ancestor(vertex), vertex, "each is its own ancestor");
    for (std::size_t other = 0; other < 5; ++other) {
      check_equal(untouched.is_connected(vertex, other), vertex == other,
                  "only itself, " + at(5, vertex, other));
    }
  }

  ds::disjoint_set_union empty_structure(0);
  check_equal(empty_structure.get_component_count(), std::size_t(0), "no vertices, no components");
}

// Reading a structure through a const reference is the whole point of making the queries const,
// and it is a property of the type rather than of any value, so it belongs in a test that would
// stop compiling if it were lost.
std::size_t count_connected_to(const ds::disjoint_set_union& structure, std::size_t size,
                               std::size_t vertex) {
  std::size_t count = 0;
  for (std::size_t other = 0; other < size; ++other) {
    count += structure.is_connected(vertex, other) ? 1 : 0;
  }
  return count;
}

void const_usability() {
  testing::section("const usability");
  testing::random_source source(seed ^ 0x6666666666666666ULL);

  const std::size_t size = 100;
  ds::disjoint_set_union built(size);
  for (std::size_t round = 0; round < 60; ++round) {
    built.merge(source.below(size), source.below(size));
  }

  const ds::disjoint_set_union& frozen = built;
  for (std::size_t vertex = 0; vertex < size; ++vertex) {
    check_equal(count_connected_to(frozen, size, vertex), frozen.get_component_size(vertex),
                "a const reference answers, and agrees with itself, at " + std::to_string(vertex));
    check_equal(frozen.get_ancestor(vertex), built.get_ancestor(vertex),
                "the const and non-const views give the same ancestor");
  }
  check(frozen.get_component_count() > 0, "counting through a const reference works");
}

// The storage is mutable, which is exactly the situation where a shallow copy would go
// unnoticed: both structures would answer correctly right up until one of them merged.
void value_semantics() {
  testing::section("value semantics");
  testing::random_source source(seed ^ 0x7777777777777777ULL);

  const std::size_t size = 64;
  ds::disjoint_set_union original(size);
  for (std::size_t round = 0; round < 30; ++round) {
    original.merge(source.below(size), source.below(size));
  }

  ds::disjoint_set_union copy = original;
  check_equal(copy.get_component_count(), original.get_component_count(),
              "a copy starts with the same component count");

  // Snapshot the original's whole relation, then collapse the copy. If the two shared storage
  // the snapshot would stop describing the original, which is the failure `mutable` invites.
  std::vector<bool> relation(size * size, false);
  for (std::size_t first = 0; first < size; ++first) {
    for (std::size_t second = 0; second < size; ++second) {
      relation[first * size + second] = original.is_connected(first, second);
    }
  }
  std::size_t original_count_before = original.get_component_count();

  for (std::size_t vertex = 0; vertex + 1 < size; ++vertex) {
    copy.merge(vertex, vertex + 1);
  }
  check_equal(copy.get_component_count(), std::size_t(1), "the copy collapsed to one component");
  check_equal(original.get_component_count(), original_count_before,
              "the original is untouched by merges in its copy");
  for (std::size_t first = 0; first < size; ++first) {
    for (std::size_t second = 0; second < size; ++second) {
      check_equal(original.is_connected(first, second), relation[first * size + second],
                  "the original's relation is unchanged, " + at(size, first, second));
    }
  }

  // Compression in one must not reach the other either, and a copy taken before any path was
  // read must answer exactly like one taken after every path was read.
  ds::disjoint_set_union uncompressed(size);
  testing::random_source replay(seed ^ 0x8888888888888888ULL);
  for (std::size_t round = 0; round < 30; ++round) {
    uncompressed.merge(replay.below(size), replay.below(size));
  }
  ds::disjoint_set_union before_reading = uncompressed;
  for (std::size_t vertex = 0; vertex < size; ++vertex) {
    uncompressed.get_ancestor(vertex);
  }
  ds::disjoint_set_union after_reading = uncompressed;
  for (std::size_t first = 0; first < size; ++first) {
    for (std::size_t second = 0; second < size; ++second) {
      check_equal(
          after_reading.is_connected(first, second), before_reading.is_connected(first, second),
          "compression before a copy changes nothing it answers, " + at(size, first, second));
    }
  }

  ds::disjoint_set_union moved = std::move(after_reading);
  check_equal(moved.get_component_count(), before_reading.get_component_count(),
              "a moved structure carries its partition across");
}

// Relations that must hold no matter which merges happened. These are cheap, and each one fails
// differently: a drifting counter, a size read off a stale non-root, an asymmetric answer.
void invariants_after_arbitrary_merges() {
  testing::section("invariants after arbitrary merges");
  testing::random_source source(seed ^ 0x9999999999999999ULL);

  const std::size_t size = 200;
  ds::disjoint_set_union structure(size);

  for (std::size_t round = 0; round <= 3 * size; ++round) {
    if (round > 0) {
      structure.merge(source.below(size), source.below(size));
    }

    std::vector<std::size_t> ancestors(size);
    for (std::size_t vertex = 0; vertex < size; ++vertex) {
      ancestors[vertex] = structure.get_ancestor(vertex);
    }

    std::vector<bool> counted(size, false);
    std::size_t distinct = 0;
    std::size_t summed = 0;
    for (std::size_t vertex = 0; vertex < size; ++vertex) {
      if (!counted[ancestors[vertex]]) {
        counted[ancestors[vertex]] = true;
        ++distinct;
        summed += structure.get_component_size(vertex);
      }
    }
    check_equal(distinct, structure.get_component_count(),
                "distinct ancestors equal the component count at round " + std::to_string(round));
    check_equal(summed, size, "component sizes sum to n at round " + std::to_string(round));

    for (std::size_t vertex = 0; vertex < size; ++vertex) {
      std::size_t sharing = 0;
      for (std::size_t other = 0; other < size; ++other) {
        sharing += ancestors[other] == ancestors[vertex] ? 1 : 0;
      }
      check_equal(
          structure.get_component_size(vertex), sharing,
          "a size counts exactly the vertices sharing an ancestor, round " + std::to_string(round));
      check(structure.is_connected(vertex, vertex), "connectedness is reflexive");
    }
  }
}

// The counter is the one piece of state not rebuilt from the parents, so it can drift silently.
// Tie it to merge's own return value rather than to the reference.
void counter_follows_merges() {
  testing::section("the counter follows merges");
  testing::random_source source(seed ^ 0xaaaaaaaaaaaaaaaaULL);

  const std::size_t size = 80;
  ds::disjoint_set_union structure(size);
  check_equal(structure.get_component_count(), size, "an untouched structure has n components");

  std::size_t expected = size;
  for (std::size_t round = 0; round < 5 * size; ++round) {
    std::size_t first = source.below(size);
    std::size_t second = source.below(size);
    bool joined = structure.merge(first, second);
    if (joined) {
      --expected;
    }
    check_equal(
        structure.get_component_count(), expected,
        "one successful merge removes exactly one component, round " + std::to_string(round));
    check(!structure.merge(first, second), "merging the same pair again joins nothing");
    check(!structure.merge(second, first), "and neither does merging it the other way");
    check_equal(structure.get_component_count(), expected,
                "a merge that joined nothing leaves the count alone");
    check(structure.is_connected(first, second), "the pair is connected afterwards either way");
    check_equal(structure.is_connected(first, second), structure.is_connected(second, first),
                "connectedness is symmetric");
  }
  check_equal(expected, structure.get_component_count(), "the count never drifted");
}

void star_and_full_collapse() {
  testing::section("star and full collapse");

  // A star: one vertex absorbs every other. Union by size keeps attaching the singleton to the
  // growing component rather than the reverse.
  const std::size_t size = 500;
  ds::disjoint_set_union star(size);
  for (std::size_t vertex = 1; vertex < size; ++vertex) {
    check(star.merge(0, vertex), "each spoke of a star joins something new");
    check_equal(star.get_component_size(0), vertex + 1, "the star grows by one each time");
    check_equal(star.get_component_count(), size - vertex, "and loses one component each time");
  }
  for (std::size_t vertex = 0; vertex < size; ++vertex) {
    check_equal(star.get_component_size(vertex), size,
                "every vertex of a collapsed structure reports the whole size");
    check_equal(star.get_ancestor(vertex), star.get_ancestor(0),
                "and shares one ancestor with every other");
  }
  check_equal(star.get_component_count(), std::size_t(1), "a star is one component");

  // Self merges at scale must be inert.
  ds::disjoint_set_union inert(100);
  for (std::size_t round = 0; round < 1000; ++round) {
    check(!inert.merge(round % 100, round % 100), "a self merge joins nothing");
  }
  check_equal(inert.get_component_count(), std::size_t(100), "and leaves every vertex alone");
}

// Sizes near 1e3 against the same quadratic reference, which makes this section O(n^2).
void large_against_quadratic() {
  testing::section("large, checked against the quadratic reference");
  const std::vector<std::size_t> sizes = {997, 1000, 1024};

  for (std::size_t size : sizes) {
    testing::random_source source(seed ^ (0x5555555555555555ULL + size));
    ds::disjoint_set_union structure(size);
    labelled_partition reference(size);

    for (std::size_t round = 0; round < 4 * size; ++round) {
      if (source.below(4) == 0) {
        std::size_t first = source.below(size);
        std::size_t second = source.below(size);
        check_equal(structure.is_connected(first, second), reference.is_connected(first, second),
                    "is_connected " + at(size, first, second));
        check_equal(structure.get_component_size(first), reference.component_size(first),
                    "get_component_size at " + std::to_string(first));
        continue;
      }
      std::size_t first = source.below(size);
      std::size_t second = source.below(size);
      check_equal(structure.merge(first, second), reference.merge(first, second),
                  "merge " + at(size, first, second));
    }

    check_equal(structure.get_component_count(), reference.component_count(),
                "component count after the whole run, n=" + std::to_string(size));
    for (std::size_t vertex = 0; vertex < size; ++vertex) {
      check_equal(structure.get_component_size(vertex), reference.component_size(vertex),
                  "final component size at " + std::to_string(vertex));
    }
  }
}

}  // namespace

int main() {
  exhaustive_small();
  ancestor_contract();
  const_usability();
  value_semantics();
  invariants_after_arbitrary_merges();
  counter_follows_merges();
  star_and_full_collapse();
  shapes_that_stress_the_path();
  singletons_and_self_merges();
  large_against_quadratic();

  return testing::summarize("disjoint_set_union") == 0 ? 0 : 1;
}
