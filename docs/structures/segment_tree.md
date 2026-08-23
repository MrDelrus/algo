# segment_tree

## Summary

Point assignment and range fold over an arbitrary monoid.

## Complexity

| Operation | Time |
| --- | --- |
| `segment_tree(size)` | O(n) |
| `segment_tree(values)` | O(n) |
| `get` | O(1) |
| `set` | O(log n) |
| `combine_at` | O(log n) |
| `query` | O(log n) |
| `query_all` | O(1) |

Memory is `2 * ceil_pow2(n)` values. The leaf count is rounded up to a power of two, and the padding leaves hold `identity()`, which is why `query(0, n)` and `query_all()` agree.

Measured throughput is in [Performance](#performance) below.

## The monoid

A monoid supplies a value type, an associative `combine`, and a two-sided `identity`:

```cpp
template <typename value>
struct sum_monoid {
  using value_type = value;
  static constexpr value_type identity() { return value_type(0); }
  static constexpr value_type combine(const value_type& left, const value_type& right) {
    return left + right;
  }
};
```

**Commutativity is not required.** `query` folds the left part left-to-right and the right part right-to-left and keeps operand order, so monoids like matrix composition or "last assignment wins" behave correctly.

Four are provided — `sum_monoid`, `min_monoid`, `max_monoid`, `gcd_monoid` — with identities `0`, `numeric_limits::max()`, `numeric_limits::lowest()`, and `0` respectively. A custom one is the five lines above.

Monoids live directly in `algo::data_structures`, one level above the tree's own namespace, because every structure parameterized by an operation reuses them — `ds::min_monoid<std::int64_t>` is the same type whether a segment tree or a sparse table consumes it.

## API

```cpp
namespace algo::data_structures::segment_trees {

template <typename monoid>
class segment_tree {
 public:
  using value_type = typename monoid::value_type;

  segment_tree();
  explicit segment_tree(std::size_t size);
  explicit segment_tree(const std::vector<value_type>& values);

  value_type get(std::size_t position) const;
  void set(std::size_t position, const value_type& value);
  void combine_at(std::size_t position, const value_type& value);

  value_type query(std::size_t left, std::size_t right) const;
  value_type query_all() const;
};

}  // namespace algo::data_structures::segment_trees
```

Positions are 0-indexed, ranges are half-open `[left, right)`.

| Member | Meaning |
| --- | --- |
| `segment_tree()` | Empty tree. Useful when the size is known only later. |
| `segment_tree(size)` | `size` elements, all equal to `identity()`. |
| `segment_tree(values)` | Built from `values`, bottom up. |
| `get(position)` | The element at `position`. |
| `set(position, value)` | Assigns `value`, ignoring what was there. |
| `combine_at(position, value)` | `a[position] = combine(a[position], value)`. Adding to an element in a sum tree, taking a minimum with it in a min tree. |
| `query(left, right)` | Fold of `[left, right)`. Returns `identity()` when `left == right`. |
| `query_all()` | Fold of the whole array, read straight from the root. Equal to `query(0, n)`. |

## Usage

```cpp
std::vector<std::int64_t> values = {5, 1, 4, 1, 9, 2, 6};

ds::sum_segment_tree tree(values);
tree.set(2, 10);            // values[2] = 10
tree.combine_at(0, 3);      // values[0] += 3
std::int64_t total = tree.query(1, 5);

ds::min_segment_tree minimums(values);
std::int64_t smallest = minimums.query(0, 4);
```

The class lives in `algo::data_structures::segment_trees`, the family namespace it will share with `lazy_segment_tree`. Aliases for the presets are declared one level up, alongside every other structure the library offers: `ds::sum_segment_tree`, `ds::min_segment_tree`, `ds::max_segment_tree`, `ds::gcd_segment_tree`, all over `std::int64_t`.

Reach past an alias only for a monoid that has no preset:

```cpp
ds::segment_trees::segment_tree<ds::min_monoid<std::int32_t>> tree(values);
ds::segment_trees::segment_tree<my_monoid> custom(values);
```

## Notes

**Ancestors are recomputed, never patched.** After a leaf changes, each ancestor is rebuilt as `combine(left_child, right_child)`. The tempting shortcut — folding the new value into every ancestor directly — is correct only for commutative operations and would silently corrupt a matrix or assignment monoid. Both cost one `combine` per level, so the safe version is free.

This is also why `combine_at` needs no inverse and works for `min`, `max`, and `gcd`, none of which have one. Structures that genuinely require an inverse, `fenwick` among them, will take a stronger contract than a monoid.

**Every entry point validates its arguments** and throws `std::out_of_range` for a position at or past `n`, a range reaching past `n`, or a reversed range. A satisfied check is one perfectly predicted branch, and the message is only built on the failing path. `query(5, 5)` on a tree of size 5 is legal and returns `identity()`; `query(4, 2)` is a bug and throws.

**Overflow is the caller's problem.** `sum_monoid<std::int64_t>` will wrap silently if the total exceeds 64 bits; nothing in the tree checks for it.

**When not to use it.** For point updates and prefix sums only, `fenwick` is smaller, faster by a constant factor, and shorter to type. Reach for `segment_tree` when the operation has no inverse, or when ranges are arbitrary rather than prefixes.

## Performance

Source: [benchmarks/structures/segment_tree.cpp](../../benchmarks/structures/segment_tree.cpp), run with `benchmarks/run.sh structures/segment_tree`.

GCC 13.2.0, `-std=c++20 -O2 -static`, in a container pinned to one core with 256 MB and no network. `n = 200 000`, fastest of 5 runs. Building the tree from a vector takes **0.57 ms**.

Each scenario is a fixed mix of operations at random positions, drawn from a counter-based splitmix64 with a fixed seed — a rerun performs bitwise identical work. The generator column is the same stream drawn without touching the tree, so the harness cost is visible rather than folded into the total.

| Scenario | Mix | Operations | Total | Generator | Spread |
| --- | --- | --- | --- | --- | --- |
| 1 — one hot position | `get` 5/10, `set` 5/10, all aimed at a single position drawn once | 20 000 000 | 360.08 ms | 150.38 ms | 1.2% |
| 2 — balanced | `set` 2/10, `combine_at` 2/10, `get` 2/10, `query` 3/10, `query_all` 1/10 | 20 000 000 | 1475.30 ms | 321.83 ms | 4.9% |
| 3 — query heavy | `set` 1/10, `combine_at` 1/10, `get` 1/10, `query` 5/10, `query_all` 2/10 | 20 000 000 | 1663.24 ms | 358.03 ms | 5.7% |

Scenario 1 keeps the whole root-to-leaf path in cache; scenarios 2 and 3 miss on every access. The tree executes the same instructions in all three, so a fourfold difference in total time is memory, not work — which is the useful thing to know when a solution is close to the limit. Moving weight from point updates onto range queries costs another 13%, since a query descends from both ends and touches two paths instead of one.

Twenty million mixed operations on a tree of 2e5 finish inside two seconds, so a problem with 2e5 queries spends a few tens of milliseconds inside the tree.

Only the total is reported. Splitting it across operation kinds was tried and dropped — the jitter between two passes is a few percent, and dividing that by the calls of a one-in-ten operation leaves several nanoseconds of uncertainty per call, more than the cheap operations cost. What a single operation costs is what its complexity says it costs.

Absolute times are not comparable to a Codeforces verdict; the judge's CPU and scheduler are different. They compare our implementations against each other under identical conditions.

## Related

- [documentation index](../README.md)
- [tests](../../tests/structures/segment_tree.cpp)
- [benchmark source](../../benchmarks/structures/segment_tree.cpp)
