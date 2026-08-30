# Documentation index

One page per component, describing that implementation for a reader who already knows the structure: which variant and heuristics were used, the complexity and memory, what each operation does, and the conventions it obeys.

## Data structures — `algo::data_structures`, alias `ds`

| Component | Documentation | Tests |
| --- | --- | --- |
| `heap_min`, `heap_max`, `heapify` | [structures/heap.md](structures/heap.md) | [source](../tests/structures/heap.cpp) |
| `disjoint_set_union` | [structures/disjoint_set_union.md](structures/disjoint_set_union.md) | [source](../tests/structures/disjoint_set_union.cpp) |
| `segment_tree` | [structures/segment_tree.md](structures/segment_tree.md) | [source](../tests/structures/segment_tree.cpp) |

## Graphs — `algo::graphs`, alias `gr`

| Component | Documentation | Tests |
| --- | --- | --- |
| `dijkstra`, `bellman_ford`, `get_components` | [graphs/shortest_paths.md](graphs/shortest_paths.md) | [source](../tests/graphs/shortest_paths.cpp) |

## Conventions

These hold on every page unless it says otherwise.

- Positions and vertices are 0-indexed; ranges are half-open `[left, right)`.
- Values and weights are `std::int64_t`.
- Structures own their storage. Arguments are not validated: misuse is undefined behaviour, as in the standard library.
- Anything parameterized by an operation takes a monoid — `value_type`, `identity()`, `combine(left, right)` — with an explicit identity, never a default-constructed one.
- Graph traversals are iterative: a Codeforces stack does not survive 2e5 frames.
