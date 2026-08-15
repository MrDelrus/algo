# algo::graphs

Graph algorithms. Alias: `gr`.

## Components

| Component | Page | Complexity | Status |
| --- | --- | --- | --- |
| _none yet_ | | | |

## Conventions

- Vertices are `std::int64_t`, 0-indexed, numbered `0 .. n - 1`.
- Weights are `std::int64_t`; unreachable distance is `INF`.
- Adjacency is stored as `vector<vector<...>>` unless a component documents otherwise.
- Directed and undirected variants are separate entry points, never a runtime flag.
- Nothing recurses to depth `n` — traversals are iterative, since Codeforces stacks blow up around 2e5 frames.
