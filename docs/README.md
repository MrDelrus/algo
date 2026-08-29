# Documentation index

One page per component. Every page has the same sections: summary, complexity, API, usage, notes, related. Speed is stated as complexity, not as measurements: a recorded number describes one afternoon on one machine, and nothing keeps it true.

## Data structures — `algo::data_structures`, alias `ds`

| Component | Documentation | Tests |
| --- | --- | --- |
| `cartesian_tree` | [structures/cartesian_tree.md](structures/cartesian_tree.md) | [source](../tests/structures/cartesian_tree.cpp) |
| `disjoint_set_union` | [structures/disjoint_set_union.md](structures/disjoint_set_union.md) | [source](../tests/structures/disjoint_set_union.cpp) |
| `segment_tree` | [structures/segment_tree.md](structures/segment_tree.md) | [source](../tests/structures/segment_tree.cpp) |

## Graphs — `algo::graphs`, alias `gr`

| Component | Documentation | Tests |
| --- | --- | --- |
| _none yet_ | | |

## Conventions

These hold on every page unless it says otherwise.

- Positions and vertices are 0-indexed; ranges are half-open `[left, right)`.
- Values and weights are `std::int64_t`.
- Structures own their storage and validate their arguments, throwing `std::out_of_range` or `std::invalid_argument` on misuse.
- Anything parameterized by an operation takes a monoid — `value_type`, `identity()`, `combine(left, right)` — with an explicit identity, never a default-constructed one.
- Graph traversals are iterative: a Codeforces stack does not survive 2e5 frames.
