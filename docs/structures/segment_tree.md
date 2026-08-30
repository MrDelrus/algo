# segment_tree

Point assignment and range fold over a monoid.

```cpp
ds::segment_tree_sum tree(values);          // also _min, _max, _gcd, over std::int64_t
ds::segment_trees::segment_tree<my_monoid> custom(values);
```

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
| `combine_at(position, value)` | `a[position] = combine(a[position], value)`. Adds in a sum tree, takes a minimum in a min tree. **This is the one to use for "add x at position i"** — `set` would overwrite. |
| `query(left, right)` | Fold of `[left, right)`. Returns `identity()` when `left == right`. |
| `query_all()` | Fold of everything. Same answer as `query(0, n)`. |

Everything throws `std::out_of_range` for a position at or past `n`, a range reaching past `n`, or a reversed range. `query(5, 5)` on a tree of five is legal; `query(4, 2)` is not.

There is no `size()` — the size is fixed at construction and the caller already knows it.

## The monoid

Three names, and that is the whole contract:

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

**Operand order is preserved**, so a non-commutative `combine` — matrix products, "last write wins" — gives the answer the positions imply.

## Traps

- `sum_monoid<std::int64_t>` wraps silently on overflow.
- `combine_at` needs no inverse, so it works for `min`, `max` and `gcd`.

## Related

- [documentation index](../README.md)
- [tests](../../tests/structures/segment_tree.cpp)
