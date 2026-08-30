# disjoint_set_union

A partition of `0 .. n - 1` under merging.

```cpp
ds::disjoint_set_union structure(n);
```

## Operations

Vertices are **0-indexed**.

| | |
| --- | --- |
| `merge(first, second)` | Joins their components. **Returns `false` when they already shared one.** |
| `is_connected(first, second)` | Whether they share a component. |
| `get_ancestor(vertex)` | The component's representative. |
| `get_component_size(vertex)` | How many vertices are in this vertex's component. |
| `get_component_count()` | How many components remain. |

All of them throw `std::out_of_range` for a vertex at or past `n`. The default constructor gives an empty structure with no vertices at all.

## Traps

- **The return value of `merge` is Kruskal's cycle test**, with no second traversal:

  ```cpp
  for (auto [weight, from, to] : edges) {   // sorted by weight
    if (structure.merge(from, to)) {
      total += weight;
    }
  }
  ```

- **An ancestor is an unspecified vertex of its component** — not the smallest, not the first inserted — and it changes as merges happen. Guaranteed: it belongs to the component, it is its own ancestor, and two vertices share one exactly when connected. Do not store it across a `merge`.
- **Queries are `const`** despite rewriting paths internally, so the structure can be passed as `const&`.
- **There is no split.** Undoing a merge requires giving up path compression.

## Related

- [documentation index](../README.md)
- [tests](../../tests/structures/disjoint_set_union.cpp)
