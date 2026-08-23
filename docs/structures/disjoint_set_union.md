# disjoint_set_union

## Summary

Keeps a partition of `0 .. n - 1` under merging, answering which vertices share a component in near-constant time.

## Complexity

| Operation | Time |
| --- | --- |
| `disjoint_set_union(size)` | O(n) |
| `get_ancestor` | O(α(n)) amortised |
| `merge` | O(α(n)) amortised |
| `is_connected` | O(α(n)) amortised |
| `get_component_size` | O(α(n)) amortised |
| `get_component_count` | O(1) |

Memory is `2n` values. α is the inverse Ackermann function — below 5 for any `n` that fits in memory, so every operation is a constant in practice.

## API

```cpp
namespace algo::data_structures::disjoint_set_unions {

class disjoint_set_union {
 public:
  disjoint_set_union();
  explicit disjoint_set_union(std::size_t size);

  std::size_t get_ancestor(std::size_t vertex) const;
  bool merge(std::size_t first, std::size_t second);
  bool is_connected(std::size_t first, std::size_t second) const;

  std::size_t get_component_size(std::size_t vertex) const;
  std::size_t get_component_count() const;
};

}  // namespace algo::data_structures::disjoint_set_unions
```

Vertices are 0-indexed.

| Member | Meaning |
| --- | --- |
| `disjoint_set_union()` | Empty. Useful when the size is known only later. |
| `disjoint_set_union(size)` | `size` vertices, each alone in its own component. |
| `get_ancestor(vertex)` | The representative of the vertex's component. |
| `merge(first, second)` | Joins the two components. **Returns `false` when they were already the same one.** |
| `is_connected(first, second)` | Whether the two vertices share a component. |
| `get_component_size(vertex)` | How many vertices are in this vertex's component. |
| `get_component_count()` | How many components are left. |

### What an ancestor is, and is not

`get_ancestor` returns *some* vertex of the component — which one is unspecified and changes as merges happen. What is guaranteed: it belongs to the component, it is its own ancestor, and two vertices share an ancestor exactly when they are connected. Do not store it across a `merge` and do not expect it to be the smallest vertex or the first one merged.

### Why `merge` returns a `bool`

It reports whether anything was actually joined. Kruskal's algorithm is the reason: an edge whose endpoints already share a component closes a cycle and must be skipped, and this is that test without a second traversal.

```cpp
std::int64_t weight_total = 0;
for (auto [weight, from, to] : edges) {          // sorted by weight
  if (structure.merge(from, to)) {
    weight_total += weight;
  }
}
```

## Usage

```cpp
ds::disjoint_set_union structure(n);

structure.merge(a, b);
if (structure.is_connected(a, c)) { /* ... */ }

std::size_t largest = 0;
for (std::size_t vertex = 0; vertex < n; ++vertex) {
  largest = std::max(largest, structure.get_component_size(vertex));
}

bool everything_is_one_piece = structure.get_component_count() == 1;
```

## Notes

**Queries are `const`, and they do rewrite memory.** `get_ancestor` compresses the path it walks, attaching every vertex on it straight to the root. That changes how the partition is stored, never which partition it is, so it is invisible in the model this class presents — the same reason a splay tree's lookup is conceptually a query. The storage is `mutable` to say exactly that. A `disjoint_set_union` can therefore be passed as `const&` and still answer questions at full speed.

**Union by size, not by rank.** The bound is identical: Tarjan and van Leeuwen showed that any balanced union rule combined with any path-shortening rule gives O(α(n)), because both rules keep tree height O(log n) even before compression. Size wins on usefulness — problems ask how big a component is, and no problem asks for a rank. After compression a rank is no longer a real height anyway, while a size stays exact.

**Both heuristics are load-bearing.** Union by size alone gives O(log n). Path compression alone gives O(log n). Only together do they give α.

**No recursion.** The path to a root is walked with two loops. A recursive `find` on 2e5 vertices overflows the Codeforces stack, and that failure looks like a runtime error on a random test rather than a stack trace.

**Merging is one-way.** There is no split, and adding one would cost the heuristics: an undo-capable variant has to give up path compression and settle for O(log n). If a problem needs rollback, that is a different structure and it will live beside this one in `disjoint_set_unions`.

**Every entry point validates its vertices** and throws `std::out_of_range`. A satisfied check is one perfectly predicted branch, and the message is built only on the failing path.

## Related

- [documentation index](../README.md)
- [tests](../../tests/structures/disjoint_set_union.cpp)
