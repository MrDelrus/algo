# segment_tree

## Summary

Point assignment and range fold over an arbitrary monoid, with an O(log n) descent that finds how far a range can be extended before a predicate stops holding.

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
| `find_right`, `find_left` | O(log n) |

Memory is `2 * ceil_pow2(n)` values. The leaf count is rounded up to a power of two so that the descent is a plain walk down the tree; on an unpadded layout the leaves are rotated and the descent becomes error-prone. Padding leaves hold `identity()`, which is why `query(0, n)` and `query_all()` agree.

Measured throughput is in [benchmarks/structures/segment_tree.md](../../benchmarks/structures/segment_tree.md).

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

Monoids live directly in `algo::data_structures`, not inside the tree, because every structure parameterized by an operation will reuse them.

## API

```cpp
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

  template <typename predicate>
  std::size_t find_right(std::size_t left, predicate is_good) const;
  template <typename predicate>
  std::size_t find_left(std::size_t right, predicate is_good) const;
};
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
| `find_right(left, is_good)` | The largest `right` in `[left, n]` with `is_good(query(left, right))` true. |
| `find_left(right, is_good)` | The smallest `left` in `[0, right]` with `is_good(query(left, right))` true. |

### The descent

The predicate is applied to the **fold of a candidate range**, never to a single element or an index.

`find_right` fixes the left end and grows the range rightwards until the predicate breaks; `find_left` fixes the right end and grows leftwards. Both return exactly the index that goes into `query` in its own place, so the found range is `[left, find_right(left, …))` or `[find_left(right, …), right)` with no adjustment by one.

Two preconditions:

- `is_good(identity())` must be true — the empty range has to qualify, otherwise no answer exists. Violating this throws `std::invalid_argument`.
- The predicate must be monotone: once false, it stays false as the range keeps growing.

Accumulation order is part of the contract, since non-commutative monoids are supported. `find_right` accumulates as `combine(accumulated, node)`, `find_left` as `combine(node, accumulated)`; in both, the accumulator equals the genuine fold of the current range.

## Usage

```cpp
std::vector<std::int64_t> values = {5, 1, 4, 1, 9, 2, 6};

ds::sum_segment_tree tree(values);
tree.set(2, 10);            // values[2] = 10
tree.combine_at(0, 3);      // values[0] += 3
std::int64_t total = tree.query(1, 5);

// How far does a prefix starting at 1 stay within a budget?
std::size_t right = tree.find_right(1, [](std::int64_t sum) { return sum <= 20; });

// The first position at or after 2 holding a value below 4.
ds::min_segment_tree minimums(values);
std::size_t position = minimums.find_right(2, [](std::int64_t smallest) { return smallest >= 4; });
```

Aliases: `sum_segment_tree`, `min_segment_tree`, `max_segment_tree`, `gcd_segment_tree`, all over `std::int64_t`. For anything else, name the core: `ds::segment_tree<ds::min_monoid<std::int32_t>>`.

## Notes

**Ancestors are recomputed, never patched.** After a leaf changes, each ancestor is rebuilt as `combine(left_child, right_child)`. The tempting shortcut — folding the new value into every ancestor directly — is correct only for commutative operations and would silently corrupt a matrix or assignment monoid. Both cost one `combine` per level, so the safe version is free.

This is also why `combine_at` needs no inverse and works for `min`, `max`, and `gcd`, none of which have one. Structures that genuinely require an inverse, `fenwick` among them, will take a stronger contract than a monoid.

**Every entry point validates its arguments.** `std::out_of_range` for a position at or past `n`, a range reaching past `n`, or a reversed range; `std::invalid_argument` for a predicate that rejects `identity()`. A satisfied check is one perfectly predicted branch, and the message is only built on the failing path. `query(5, 5)` on a tree of size 5 is legal and returns `identity()`; `query(4, 2)` is a bug and throws.

**Overflow is the caller's problem.** `sum_monoid<std::int64_t>` will wrap silently if the total exceeds 64 bits; nothing in the tree checks for it.

**When not to use it.** For point updates and prefix sums only, `fenwick` is smaller, faster by a constant factor, and shorter to type. Reach for `segment_tree` when the operation has no inverse, when ranges are arbitrary rather than prefixes, or when the descent is what the problem actually needs.

## Related

- [documentation index](../README.md)
- [benchmark results](../../benchmarks/structures/segment_tree.md)
