// Correctness tests for algo::data_structures::cartesian_tree.
//
// The reference is std::set: same ordering, same uniqueness, same answers to insert, erase and
// contains. Everything the tree claims about membership is checked against it. What std::set
// cannot mirror — split, merge, and the ownership rules around them — is checked against the
// definitions directly.
//
// Everything is deterministic. Priorities come from a seed fixed in the test, so a failure
// reproduces byte for byte; the tree's default constructor seeds from the clock instead, and
// that path is exercised too but never relied on for an expected value.

#include "algo_library.hpp"

#include "../harness.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ct = algo::data_structures::cartesian_trees;

namespace {

using testing::check;
using testing::check_does_not_throw;
using testing::check_equal;
using testing::check_throws;

constexpr std::uint64_t seed = 0x2545f4914f6cdd1dULL;

std::string at(std::size_t size, std::int64_t key) {
  return "n=" + std::to_string(size) + " key=" + std::to_string(key);
}

// A key that offers operator< and nothing else — no ==, no arithmetic, no hash. If the tree ever
// reaches for equality directly this stops compiling, which is the point: std::set requires only
// a strict weak ordering and so must we.
struct ordered_only {
  std::int64_t value;
  bool operator<(const ordered_only& other) const {
    return value < other.value;
  }
};

// Every key in the domain, compared against the reference. Cheap at these sizes and it catches a
// key that quietly moved to the wrong side of a split.
void check_membership(const ds::cartesian_tree<std::int64_t>& tree,
                      const std::set<std::int64_t>& reference, std::int64_t domain,
                      const std::string& note) {
  check_equal(tree.size(), reference.size(), note + ": size");
  check_equal(tree.empty(), reference.empty(), note + ": empty");
  for (std::int64_t key = -2; key <= domain + 1; ++key) {
    check_equal(tree.contains(key), reference.count(key) != 0,
                note + ": contains " + std::to_string(key));
  }
}

// The aggregate is defined by the values in the subtree, so the reference is a std::map of key
// to value folded by hand. Slow and obviously right.
template <typename monoid>
typename monoid::value_type fold(const std::map<std::int64_t, typename monoid::value_type>& values,
                                 std::int64_t lower, std::int64_t upper) {
  typename monoid::value_type folded = monoid::identity();
  for (const auto& entry : values) {
    if (!(entry.first < lower) && entry.first < upper) {
      folded = monoid::combine(folded, entry.second);
    }
  }
  return folded;
}

constexpr std::int64_t no_bound_low = -1000000;
constexpr std::int64_t no_bound_high = 1000000;

void aggregates_follow_every_operation() {
  testing::section("aggregates follow every operation");
  testing::random_source source(seed ^ 0x5555555555555555ULL);
  using monoid = ds::sum_monoid<std::int64_t>;

  const std::int64_t domain = 30;
  ds::cartesian_tree<std::int64_t, monoid> tree(seed);
  std::map<std::int64_t, std::int64_t> reference;

  check_equal(tree.aggregate(), std::int64_t(0), "an empty tree folds to the identity");

  for (std::size_t round = 0; round < 600; ++round) {
    std::int64_t key = static_cast<std::int64_t>(source.below(static_cast<std::size_t>(domain)));
    if (source.next() % 3 == 0) {
      bool erased = tree.erase(key);
      check_equal(erased, reference.erase(key) != 0, "erase agrees with the reference");
    } else {
      std::int64_t value = static_cast<std::int64_t>(source.below(1000)) - 500;
      bool inserted = tree.insert(key, value);
      check_equal(inserted, reference.emplace(key, value).second, "insert agrees");
    }
    check_equal(tree.size(), reference.size(), "size agrees");
    check_equal(tree.aggregate(), fold<monoid>(reference, no_bound_low, no_bound_high),
                "the fold of the whole tree agrees, round " + std::to_string(round));
  }
}

// Splitting out a range and reading its aggregate is how a range query is spelled, so that is
// what gets checked — at every cut, against the folded reference.
void split_carries_the_aggregate() {
  testing::section("split carries the aggregate");
  testing::random_source source(seed ^ 0x6666666666666666ULL);
  using monoid = ds::sum_monoid<std::int64_t>;

  const std::int64_t domain = 24;
  std::map<std::int64_t, std::int64_t> reference;
  for (std::size_t round = 0; round < 18; ++round) {
    std::int64_t key = static_cast<std::int64_t>(source.below(static_cast<std::size_t>(domain)));
    reference.emplace(key, static_cast<std::int64_t>(source.below(200)) - 100);
  }

  for (std::int64_t cut = -2; cut <= domain + 1; ++cut) {
    ds::cartesian_tree<std::int64_t, monoid> tree(seed + static_cast<std::uint64_t>(cut));
    for (const auto& entry : reference) {
      tree.insert(entry.first, entry.second);
    }

    auto halves = ct::split(std::move(tree), cut);
    check_equal(halves.first.aggregate(), fold<monoid>(reference, no_bound_low, cut),
                "the left half folds its own keys, cut=" + std::to_string(cut));
    check_equal(halves.second.aggregate(), fold<monoid>(reference, cut, no_bound_high),
                "the right half folds the rest, cut=" + std::to_string(cut));

    auto rebuilt = ct::merge(std::move(halves.first), std::move(halves.second));
    check_equal(rebuilt.aggregate(), fold<monoid>(reference, no_bound_low, no_bound_high),
                "and merging restores the whole fold, cut=" + std::to_string(cut));
  }
}

// A range query built from two splits, which is the pattern a problem would actually use.
void range_queries_through_split() {
  testing::section("range queries through split");
  testing::random_source source(seed ^ 0x7777777777777777ULL);
  using monoid = ds::min_monoid<std::int64_t>;

  const std::int64_t domain = 20;
  std::map<std::int64_t, std::int64_t> reference;
  for (std::int64_t key = 0; key < domain; ++key) {
    reference.emplace(key, static_cast<std::int64_t>(source.below(500)));
  }

  for (std::int64_t lower = 0; lower <= domain; ++lower) {
    for (std::int64_t upper = lower; upper <= domain; ++upper) {
      ds::cartesian_tree<std::int64_t, monoid> tree(seed);
      for (const auto& entry : reference) {
        tree.insert(entry.first, entry.second);
      }

      auto first_cut = ct::split(std::move(tree), lower);
      auto second_cut = ct::split(std::move(first_cut.second), upper);
      check_equal(second_cut.first.aggregate(), fold<monoid>(reference, lower, upper),
                  "minimum over [" + std::to_string(lower) + ", " + std::to_string(upper) + ")");

      auto tail = ct::merge(std::move(second_cut.first), std::move(second_cut.second));
      auto whole = ct::merge(std::move(first_cut.first), std::move(tail));
      check_equal(whole.size(), reference.size(), "and the tree survives being taken apart");
    }
  }
}

void aggregates_survive_copying() {
  testing::section("aggregates survive copying");
  using monoid = ds::sum_monoid<std::int64_t>;

  ds::cartesian_tree<std::int64_t, monoid> original(seed);
  for (std::int64_t key = 0; key < 20; ++key) {
    original.insert(key, key + 1);
  }
  const std::int64_t total = 20 * 21 / 2;
  check_equal(original.aggregate(), total, "the fold of one to twenty");

  ds::cartesian_tree<std::int64_t, monoid> copy = original;
  check_equal(copy.aggregate(), total, "a copy folds the same");
  copy.erase(19);
  check_equal(copy.aggregate(), total - 20, "the copy's fold follows its own erase");
  check_equal(original.aggregate(), total, "and the original's does not move");

  ds::cartesian_tree<std::int64_t, monoid> moved = std::move(copy);
  check_equal(moved.aggregate(), total - 20, "a move carries the fold across");
  check_equal(copy.aggregate(), std::int64_t(0), "and leaves the identity behind");
}

// A monoid whose combine is not commutative, to prove the fold is assembled left to right rather
// than in whatever order the tree happens to hold the nodes.
struct concat_monoid {
  using value_type = std::string;
  static value_type identity() {
    return std::string();
  }
  static value_type combine(const value_type& left, const value_type& right) {
    return left + right;
  }
};

void the_fold_respects_key_order() {
  testing::section("the fold respects key order");

  ds::cartesian_tree<std::int64_t, concat_monoid> tree(seed);
  const std::string letters = "abcdefghijklmnopqrst";
  for (std::size_t index = 0; index < letters.size(); ++index) {
    tree.insert(static_cast<std::int64_t>(index), std::string(1, letters[index]));
  }
  check_equal(tree.aggregate(), letters, "the whole tree folds into key order");

  // Inserting in a scrambled order must give the same answer: the fold follows the keys, not the
  // arrival order or the shape the priorities happened to produce.
  ds::cartesian_tree<std::int64_t, concat_monoid> scrambled(seed ^ 0x99ULL);
  const std::vector<std::size_t> order = {7, 3,  19, 0,  11, 5,  1,  17, 9,  14,
                                          2, 12, 6,  18, 4,  10, 15, 8,  13, 16};
  for (std::size_t index : order) {
    scrambled.insert(static_cast<std::int64_t>(index), std::string(1, letters[index]));
  }
  check_equal(scrambled.aggregate(), letters, "and does not depend on insertion order");

  auto halves = ct::split(std::move(scrambled), 7);
  check_equal(halves.first.aggregate(), letters.substr(0, 7), "a split half folds its own prefix");
  check_equal(halves.second.aggregate(), letters.substr(7), "and the other its own suffix");
}

// The plain tree must be untouched by the aggregate machinery: no value argument, and no space
// spent on a payload that does not exist.
void a_tree_without_a_monoid_is_unchanged() {
  testing::section("a tree without a monoid is unchanged");

  ds::cartesian_tree<std::int64_t> plain(seed);
  check(plain.insert(5), "insert still takes a key alone");
  check(!plain.insert(5), "and still reports duplicates");
  check(plain.contains(5), "and still finds it");
  check_equal(plain.size(), std::size_t(1), "and still counts");

  static_assert(!ds::cartesian_tree<std::int64_t>::aggregates,
                "a tree without a monoid must not claim to aggregate");
  static_assert(ds::cartesian_tree<std::int64_t, ds::sum_monoid<std::int64_t>>::aggregates,
                "a tree with one must");
}

void against_std_set() {
  testing::section("against std::set");
  testing::random_source source(seed);

  for (std::int64_t domain :
       {std::int64_t(1), std::int64_t(2), std::int64_t(5), std::int64_t(16), std::int64_t(40)}) {
    ds::cartesian_tree<std::int64_t> tree(seed ^ static_cast<std::uint64_t>(domain));
    std::set<std::int64_t> reference;
    check_membership(tree, reference, domain, "fresh");

    for (std::size_t round = 0; round < 12 * static_cast<std::size_t>(domain); ++round) {
      std::int64_t key = static_cast<std::int64_t>(source.below(static_cast<std::size_t>(domain)));
      if (source.next() % 3 == 0) {
        bool erased = tree.erase(key);
        check_equal(erased, reference.erase(key) != 0,
                    "erase reports the same, " + at(domain, key));
      } else {
        bool inserted = tree.insert(key);
        check_equal(inserted, reference.insert(key).second,
                    "insert reports the same, " + at(domain, key));
      }
      check_membership(tree, reference, domain, "after operation");
    }
  }
}

// Splitting is defined by where each key lands, so check exactly that, for every possible cut —
// below everything, above everything, on a key that is present, on one that is not.
void split_places_every_key() {
  testing::section("split places every key");
  testing::random_source source(seed ^ 0x1111111111111111ULL);

  const std::int64_t domain = 24;
  for (std::size_t attempt = 0; attempt < 8; ++attempt) {
    std::set<std::int64_t> reference;
    for (std::size_t round = 0; round < 20; ++round) {
      reference.insert(static_cast<std::int64_t>(source.below(static_cast<std::size_t>(domain))));
    }

    for (std::int64_t cut = -2; cut <= domain + 1; ++cut) {
      ds::cartesian_tree<std::int64_t> tree(seed + static_cast<std::uint64_t>(cut));
      for (std::int64_t key : reference) {
        tree.insert(key);
      }

      auto halves = ct::split(std::move(tree), cut);
      const ds::cartesian_tree<std::int64_t>& less = halves.first;
      const ds::cartesian_tree<std::int64_t>& not_less = halves.second;

      check_equal(tree.size(), std::size_t(0), "the source of a split is left empty");
      check(tree.empty(), "and reports itself empty");
      check_equal(less.size() + not_less.size(), reference.size(),
                  "the halves hold every key between them, cut=" + std::to_string(cut));

      for (std::int64_t key = -2; key <= domain + 1; ++key) {
        bool present = reference.count(key) != 0;
        check_equal(less.contains(key), present && key < cut,
                    "left half holds exactly the smaller keys, cut=" + std::to_string(cut) +
                        " key=" + std::to_string(key));
        check_equal(not_less.contains(key), present && !(key < cut),
                    "right half holds exactly the rest, cut=" + std::to_string(cut) +
                        " key=" + std::to_string(key));
      }
    }
  }
}

void merge_rebuilds_what_split_took_apart() {
  testing::section("merge rebuilds what split took apart");
  testing::random_source source(seed ^ 0x2222222222222222ULL);

  const std::int64_t domain = 30;
  for (std::int64_t cut = -1; cut <= domain + 1; ++cut) {
    std::set<std::int64_t> reference;
    ds::cartesian_tree<std::int64_t> tree(seed + static_cast<std::uint64_t>(cut) * 7);
    for (std::size_t round = 0; round < 25; ++round) {
      std::int64_t key = static_cast<std::int64_t>(source.below(static_cast<std::size_t>(domain)));
      reference.insert(key);
      tree.insert(key);
    }

    auto halves = ct::split(std::move(tree), cut);
    auto rebuilt = ct::merge(std::move(halves.first), std::move(halves.second));

    check_equal(halves.first.size(), std::size_t(0), "merge empties its left source");
    check_equal(halves.second.size(), std::size_t(0), "merge empties its right source");
    check_membership(rebuilt, reference, domain, "after a round trip through split and merge");
  }

  // Three-way: split twice, put it back in the other order.
  ds::cartesian_tree<std::int64_t> tree(seed);
  std::set<std::int64_t> reference;
  for (std::int64_t key = 0; key < 40; ++key) {
    tree.insert(key);
    reference.insert(key);
  }
  auto first_cut = ct::split(std::move(tree), 10);
  auto second_cut = ct::split(std::move(first_cut.second), 25);
  auto tail = ct::merge(std::move(second_cut.first), std::move(second_cut.second));
  auto whole = ct::merge(std::move(first_cut.first), std::move(tail));
  check_membership(whole, reference, 40, "three pieces joined back");
}

void merge_guards_its_precondition() {
  testing::section("merge guards its precondition");

  auto make = [](std::initializer_list<std::int64_t> keys) {
    ds::cartesian_tree<std::int64_t> tree(seed);
    for (std::int64_t key : keys) {
      tree.insert(key);
    }
    return tree;
  };

  {
    ds::cartesian_tree<std::int64_t> left = make({5, 6});
    ds::cartesian_tree<std::int64_t> right = make({1, 2});
    check_throws<std::invalid_argument>(
        [&] { return ct::merge(std::move(left), std::move(right)); },
        "left entirely above right throws");
  }
  {
    ds::cartesian_tree<std::int64_t> left = make({1, 5});
    ds::cartesian_tree<std::int64_t> right = make({4, 9});
    check_throws<std::invalid_argument>(
        [&] { return ct::merge(std::move(left), std::move(right)); }, "overlapping ranges throw");
  }
  {
    // The seam touching is the case the strict condition exists for: allowing it would put one
    // key in the tree twice.
    ds::cartesian_tree<std::int64_t> left = make({1, 4});
    ds::cartesian_tree<std::int64_t> right = make({4, 9});
    check_throws<std::invalid_argument>(
        [&] { return ct::merge(std::move(left), std::move(right)); },
        "a shared key at the seam throws");
  }
  {
    ds::cartesian_tree<std::int64_t> left = make({1, 2});
    ds::cartesian_tree<std::int64_t> right = make({3, 4});
    check_does_not_throw([&] { return ct::merge(std::move(left), std::move(right)); },
                         "strictly ordered ranges are accepted");
  }
  {
    // An empty side has no keys to violate anything.
    ds::cartesian_tree<std::int64_t> left = make({});
    ds::cartesian_tree<std::int64_t> right = make({3, 4});
    auto joined = ct::merge(std::move(left), std::move(right));
    check_equal(joined.size(), std::size_t(2), "merging an empty left keeps the right");
  }
  {
    ds::cartesian_tree<std::int64_t> left = make({3, 4});
    ds::cartesian_tree<std::int64_t> right = make({});
    auto joined = ct::merge(std::move(left), std::move(right));
    check_equal(joined.size(), std::size_t(2), "merging an empty right keeps the left");
  }
  {
    ds::cartesian_tree<std::int64_t> left = make({});
    ds::cartesian_tree<std::int64_t> right = make({});
    auto joined = ct::merge(std::move(left), std::move(right));
    check(joined.empty(), "merging two empty trees gives an empty tree");
  }
  {
    // A rejected merge must not have eaten the operands.
    ds::cartesian_tree<std::int64_t> left = make({5, 6});
    ds::cartesian_tree<std::int64_t> right = make({1, 2});
    try {
      auto joined = ct::merge(std::move(left), std::move(right));
    } catch (const std::invalid_argument&) {
    }
    check_equal(left.size(), std::size_t(2), "a rejected merge leaves its left operand alone");
    check_equal(right.size(), std::size_t(2), "and its right operand too");
    check(left.contains(5) && left.contains(6), "with its keys intact");
  }
}

// A tree that was moved out of must be empty and must still work — split hands its nodes away,
// and the leftover is a perfectly good empty tree, not a wreck.
void moved_from_trees_are_empty_and_usable() {
  testing::section("moved-from trees are empty and usable");

  ds::cartesian_tree<std::int64_t> tree(seed);
  for (std::int64_t key = 0; key < 20; ++key) {
    tree.insert(key);
  }

  auto halves = ct::split(std::move(tree), 10);
  check(tree.empty(), "a split source is empty");
  check_equal(tree.size(), std::size_t(0), "with size zero");
  for (std::int64_t key = 0; key < 20; ++key) {
    check(!tree.contains(key), "and holds nothing, key " + std::to_string(key));
  }
  check(!tree.erase(5), "erasing from it reports nothing removed");
  check(tree.insert(99), "and it accepts a fresh key");
  check_equal(tree.size(), std::size_t(1), "becoming a tree of one");

  ds::cartesian_tree<std::int64_t> taken = std::move(halves.first);
  check(halves.first.empty(), "a moved-from tree is empty");
  check(taken.contains(0), "and the destination has the contents");

  ds::cartesian_tree<std::int64_t> assigned(seed);
  assigned.insert(1000);
  assigned = std::move(halves.second);
  check(halves.second.empty(), "move assignment empties the source");
  check(!assigned.contains(1000), "and replaces what the destination held");
  check(assigned.contains(10), "with the source's keys");
}

void value_semantics() {
  testing::section("value semantics");

  ds::cartesian_tree<std::int64_t> original(seed);
  for (std::int64_t key = 0; key < 30; ++key) {
    original.insert(key);
  }

  ds::cartesian_tree<std::int64_t> copy = original;
  check_equal(copy.size(), original.size(), "a copy starts equal in size");

  copy.erase(0);
  copy.insert(1000);
  check_equal(original.size(), std::size_t(30), "the original is untouched by its copy");
  check(original.contains(0), "and still holds what the copy erased");
  check(!original.contains(1000), "and not what the copy added");

  original.erase(29);
  check(copy.contains(29), "the copy is untouched by the original");

  ds::cartesian_tree<std::int64_t> assigned(seed);
  assigned.insert(-5);
  assigned = original;
  check(!assigned.contains(-5), "copy assignment replaces the contents");
  check_equal(assigned.size(), original.size(), "and matches the source");
  assigned.erase(1);
  check(original.contains(1), "and is independent afterwards");

  original = original;
  check_equal(original.size(), std::size_t(29), "self assignment is harmless");
}

void degenerate_shapes() {
  testing::section("degenerate shapes");

  {
    ds::cartesian_tree<std::int64_t> empty_tree(seed);
    check(empty_tree.empty(), "a fresh tree is empty");
    check_equal(empty_tree.size(), std::size_t(0), "with size zero");
    check(!empty_tree.contains(0), "containing nothing");
    check(!empty_tree.erase(0), "erasing from it removes nothing");
    auto halves = ct::split(std::move(empty_tree), 0);
    check(halves.first.empty() && halves.second.empty(), "splitting it gives two empty trees");
  }
  {
    ds::cartesian_tree<std::int64_t> single(seed);
    check(single.insert(42), "one key goes in");
    check(!single.insert(42), "and does not go in twice");
    check_equal(single.size(), std::size_t(1), "size is one");
    check(single.contains(42), "it is there");
    check(single.erase(42), "it comes out");
    check(single.empty(), "leaving nothing");
    check(!single.erase(42), "and cannot come out twice");
    check(single.insert(42), "the same key can go back in");
  }
  {
    // Ascending and descending insertion are what wreck an unbalanced tree; random priorities
    // are supposed to make the order irrelevant.
    const std::int64_t count = 2000;
    ds::cartesian_tree<std::int64_t> ascending(seed);
    for (std::int64_t key = 0; key < count; ++key) {
      ascending.insert(key);
    }
    check_equal(ascending.size(), static_cast<std::size_t>(count), "ascending insertion holds all");

    ds::cartesian_tree<std::int64_t> descending(seed);
    for (std::int64_t key = count; key > 0; --key) {
      descending.insert(key);
    }
    check_equal(descending.size(), static_cast<std::size_t>(count),
                "descending insertion holds all");

    for (std::int64_t key = 0; key < count; ++key) {
      check(ascending.contains(key), "every ascending key is found");
    }
    for (std::int64_t key = 0; key < count; ++key) {
      check(descending.erase(count - key), "every descending key comes out");
    }
    check(descending.empty(), "and the tree empties");
  }
  {
    // Erasing down to nothing, one key at a time, from the middle outwards.
    ds::cartesian_tree<std::int64_t> tree(seed);
    for (std::int64_t key = 0; key < 64; ++key) {
      tree.insert(key);
    }
    for (std::int64_t step = 0; step < 32; ++step) {
      check(tree.erase(32 + step), "erasing upwards from the middle");
      check(tree.erase(31 - step), "and downwards");
      check_equal(tree.size(), static_cast<std::size_t>(62 - 2 * step), "size follows");
    }
    check(tree.empty(), "the tree ends empty");
  }
}

// Nothing in the tree may assume more of a key than std::set does.
void keys_need_only_be_ordered() {
  testing::section("keys need only be ordered");

  ds::cartesian_tree<ordered_only> tree(seed);
  check(tree.insert(ordered_only{5}), "a key with only operator< goes in");
  check(!tree.insert(ordered_only{5}), "and is recognised as already present");
  check(tree.insert(ordered_only{1}), "another goes in");
  check(tree.contains(ordered_only{5}), "and is found");
  check(!tree.contains(ordered_only{3}), "an absent one is not");
  check_equal(tree.size(), std::size_t(2), "size counts them");

  auto halves = ct::split(std::move(tree), ordered_only{4});
  check_equal(halves.first.size(), std::size_t(1), "split works on such keys too");
  check_equal(halves.second.size(), std::size_t(1), "on both sides");

  ds::cartesian_tree<std::string> words(seed);
  check(words.insert("pear"), "strings work as keys");
  check(words.insert("apple"), "in any order");
  check(!words.insert("pear"), "with the same uniqueness");
  auto split_words = ct::split(std::move(words), std::string("b"));
  check_equal(split_words.first.size(), std::size_t(1), "and split by string order");
  check(split_words.first.contains("apple"), "putting the smaller one on the left");
}

void the_default_seed_still_works() {
  testing::section("the default seed still works");

  // The clock-seeded constructor cannot be checked against an expected shape, but it must still
  // produce a correct set — and two trees built from it must agree on contents.
  ds::cartesian_tree<std::int64_t> first;
  ds::cartesian_tree<std::int64_t> second;
  std::set<std::int64_t> reference;
  testing::random_source source(seed ^ 0x3333333333333333ULL);
  for (std::size_t round = 0; round < 500; ++round) {
    std::int64_t key = static_cast<std::int64_t>(source.below(200));
    first.insert(key);
    second.insert(key);
    reference.insert(key);
  }
  check_equal(first.size(), reference.size(), "a clock-seeded tree counts correctly");
  check_equal(second.size(), reference.size(), "and so does another");
  for (std::int64_t key = 0; key < 200; ++key) {
    bool present = reference.count(key) != 0;
    check_equal(first.contains(key), present, "membership matches the reference");
    check_equal(second.contains(key), first.contains(key), "and two such trees agree");
  }
}

// Larger sizes against std::set, with split and merge folded into the stream so the tree is
// rebuilt from pieces while it is being queried.
void large_against_std_set() {
  testing::section("large, against std::set");
  testing::random_source source(seed ^ 0x4444444444444444ULL);

  const std::int64_t domain = 5000;
  ds::cartesian_tree<std::int64_t> tree(seed);
  std::set<std::int64_t> reference;

  for (std::size_t round = 0; round < 20000; ++round) {
    std::size_t choice = source.below(10);
    std::int64_t key = static_cast<std::int64_t>(source.below(static_cast<std::size_t>(domain)));

    if (choice < 4) {
      check_equal(tree.insert(key), reference.insert(key).second, "insert agrees");
    } else if (choice < 6) {
      check_equal(tree.erase(key), reference.erase(key) != 0, "erase agrees");
    } else if (choice < 9) {
      check_equal(tree.contains(key), reference.count(key) != 0, "contains agrees");
    } else {
      auto halves = ct::split(std::move(tree), key);
      check_equal(halves.first.size() + halves.second.size(), reference.size(),
                  "a split keeps every key");
      tree = ct::merge(std::move(halves.first), std::move(halves.second));
      check_equal(tree.size(), reference.size(), "and a merge puts them back");
    }
  }

  check_equal(tree.size(), reference.size(), "sizes match after the whole run");
  for (std::int64_t key = 0; key < domain; ++key) {
    check_equal(tree.contains(key), reference.count(key) != 0,
                "final membership at " + std::to_string(key));
  }
}

}  // namespace

int main() {
  against_std_set();
  aggregates_follow_every_operation();
  split_carries_the_aggregate();
  range_queries_through_split();
  aggregates_survive_copying();
  the_fold_respects_key_order();
  a_tree_without_a_monoid_is_unchanged();
  split_places_every_key();
  merge_rebuilds_what_split_took_apart();
  merge_guards_its_precondition();
  moved_from_trees_are_empty_and_usable();
  value_semantics();
  degenerate_shapes();
  keys_need_only_be_ordered();
  the_default_seed_still_works();
  large_against_std_set();

  return testing::summarize("cartesian_tree") == 0 ? 0 : 1;
}
