# segment_tree

Point assignment and range fold over a monoid.

```cpp
ds::segment_tree_sum tree(values);          // also _min, _max, _gcd, over std::int64_t
ds::segment_trees::segment_tree<my_monoid> custom(values);
```

## Implementation

Iterative, bottom-up, over a flat `std::vector`. The leaf count is rounded up to a power of two, so memory is `2 * ceil_pow2(n)` values and padding leaves hold `identity()` — which is why `query(0, n)` and `query_all()` agree.

Ancestors are recomputed from their two children after a leaf changes, never patched in place. `combine_at` therefore requires no inverse and works for `min`, `max` and `gcd`.

| | |
| --- | --- |
| build | O(n) |
| `get`, `query_all` | O(1) |
| `set`, `combine_at`, `query` | O(log n) |
| memory | `2 * ceil_pow2(n)` values |

## Constructors

| | |
| --- | --- |
| `segment_tree()` | Empty. For declaring now and filling later. |
| `segment_tree(size)` | `size` elements, every one `identity()`. |
| `segment_tree(values)` | Built from a `std::vector<value_type>`. |

## Operations

Positions are **0-indexed**, ranges are **half-open** `[left, right)`.

| | |
| --- | --- |
| `get(position)` | The element there. |
| `set(position, value)` | Assigns, ignoring what was there. |
| `combine_at(position, value)` | `a[position] = combine(a[position], value)`. Addition in a sum tree, a minimum in a min tree. Use it for "add x at position i"; `set` overwrites instead. |
| `query(left, right)` | Fold of `[left, right)`. Returns `identity()` when `left == right`. |
| `query_all()` | Fold of everything. Same answer as `query(0, n)`. |

A position at or past `n`, a range reaching past `n`, or a reversed range are **undefined behaviour** — nothing is checked, as in `std::vector::operator[]`. `query(5, 5)` on a tree of five is legal and returns `identity()`.

There is no `size()`. The size is fixed at construction.

## The monoid

The contract is three names:

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

Provided: `sum_monoid`, `min_monoid`, `max_monoid`, `gcd_monoid`, with identities `0`, `max()`, `lowest()`, `0`.

**Operand order is preserved.** A non-commutative `combine`, such as matrix multiplication, folds in position order.

## Traps

- `sum_monoid<std::int64_t>` wraps silently on overflow.
- **An out-of-range position is not diagnosed and often not even a crash.** The leaf array is padded to a power of two, so reading past `n` usually lands in padding and returns `identity()`; sanitizers see nothing. A wrong answer is the symptom.

## Related

- [documentation index](../README.md)
- [tests](../../tests/structures/segment_tree.cpp)
