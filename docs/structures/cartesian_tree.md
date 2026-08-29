# cartesian_tree

## Summary

An ordered set of unique keys that can also be cut at a key and joined back, both in O(log n).

## Complexity

| Operation | Time |
| --- | --- |
| `insert`, `erase`, `contains` | O(log n) expected |
| `split`, `merge` | O(log n) expected |
| `size`, `empty` | O(1) |
| copy | O(n) |

Every bound is **expected**, not guaranteed. A cartesian tree — a treap — is a binary search tree on keys and a heap on priorities drawn at random when a key is inserted, and it is the randomness that keeps it shallow. There are no rotations, no colour rules, and the order keys arrive in does not matter.

Memory is one node per key: the key, a 64-bit priority, a subtree size, and two owning pointers.

## Not a faster `std::set`

Measured on 200 000 insertions, 400 000 lookups and 100 000 erasures, best of seven runs pinned to one core:

| | Time |
| --- | --- |
| `std::set` | 279 ms |
| `cartesian_tree` | 466 ms |

`std::set` wins by 1.67x, and that is expected: a red-black tree *guarantees* its depth where a treap only expects it, and libstdc++ has spent decades on node allocation.

**So use `std::set` when a set is all you need.** Reach for this when the problem needs `split` or `merge` — cutting a set in two by key, or concatenating two sets — which `std::set` cannot do at all short of rebuilding.

## API

```cpp
namespace algo::data_structures::cartesian_trees {

template <typename key_type>
class cartesian_tree {
 public:
  cartesian_tree();                              // priorities seeded from the clock
  explicit cartesian_tree(std::uint64_t seed);   // priorities seeded explicitly

  cartesian_tree(const cartesian_tree&);         // deep copy
  cartesian_tree& operator=(const cartesian_tree&);
  cartesian_tree(cartesian_tree&&) noexcept;
  cartesian_tree& operator=(cartesian_tree&&) noexcept;

  bool insert(const key_type& key);              // false if the key was already there
  bool erase(const key_type& key);               // false if it was not there
  bool contains(const key_type& key) const;

  std::size_t size() const;
  bool empty() const;
};

template <typename key_type>
std::pair<cartesian_tree<key_type>, cartesian_tree<key_type>>
split(cartesian_tree<key_type>&& tree, const key_type& key);

template <typename key_type>
cartesian_tree<key_type> merge(cartesian_tree<key_type>&& left, cartesian_tree<key_type>&& right);

}  // namespace algo::data_structures::cartesian_trees
```

`split(tree, key)` returns keys `< key` on the left and keys `>= key` on the right. Either half may be empty; an empty tree is an ordinary value and needs no special handling.

`merge(left, right)` requires **every key on the left to be strictly smaller than every key on the right**, and throws `std::invalid_argument` otherwise. Strict, not `<=`: equal keys at the seam would put one key into the tree twice and silently break the uniqueness that `insert` and `erase` rely on. Two halves that came from a `split` always satisfy it.

## Usage

```cpp
ds::cartesian_tree<std::int64_t> tree;
tree.insert(5);
tree.insert(1);
tree.insert(9);

namespace ct = algo::data_structures::cartesian_trees;

auto [smaller, rest] = ct::split(std::move(tree), 5);   // {1} and {5, 9}
auto whole = ct::merge(std::move(smaller), std::move(rest));
```

## Notes

**`split` and `merge` consume their arguments**, which is why they take `&&` and why the call site needs `std::move`. That `std::move` is not ceremony: it is the only mark in the code saying the old tree is gone. A consumed tree is left **empty**, not merely unspecified — it reports `size() == 0`, contains nothing, and can be inserted into again straight away.

They consume because they must. A split reuses the very same nodes, handing them to the two halves; taking a `const&` would mean copying the whole tree at O(n) and losing exactly what the structure is for.

**Only `operator<` is required of a key**, exactly as for `std::set`. Equality is expressed as neither ordering holding, so a key type needs no `operator==`, no hash, no arithmetic.

**A tree owns its nodes** through `std::unique_ptr`, so it copies deeply and moves cheaply, and no tree can be disturbed by what happens to another. A flat arena with integer indices measured about 1.3x faster and was rejected: it would have needed one shared static pool, making the type uncopyable and tangling every tree's lifetime with every other. Since `std::set` gives away more than that for free, the price was not worth paying.

**Priorities come from the clock by default.** Pass a seed to make a run reproducible — that is what the tests do. Priorities are only ever compared inside one tree, so two trees sharing a seed cost nothing.

**Recursion depth is O(log n) expected**, around 40 levels at n = 2e5, so `split`, `merge` and `erase` recurse safely. Destruction and deep copy recurse to the same depth.

**Room left for aggregates.** Every node recomputes what it knows about its subtree in one place, `pull()`, which today maintains only the subtree size. A monoid aggregate — a sum, a minimum, a count — belongs in that same function, reached through a second template parameter with a default, which is a backwards-compatible addition rather than a change. Nothing of the sort is implemented yet, because nothing needs it yet.

**When not to use it.** For a plain set, `std::set` is faster and shorter to type. For prefix sums with point updates, `fenwick` or `segment_tree` are far cheaper. This structure earns its place when keys must be cut apart and rejoined.

## Related

- [documentation index](../README.md)
- [tests](../../tests/structures/cartesian_tree.cpp)
