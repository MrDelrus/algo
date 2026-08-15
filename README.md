# algo

Pre-written competitive programming library for Codeforces rounds.

## 1. Overview

This repository holds a personal library of standard algorithms and data structures, written ahead of time (with the help of Claude Opus 5) and published so that it can be reused during Codeforces rounds. Nothing here is round-specific: it is the reusable half of a solution — the half that should never need debugging under contest pressure.

The whole library lives in `main.cpp`, which is the single file that gets submitted. Everything is inside `namespace algo`, split into subnamespaces:

```cpp
namespace algo {
    namespace graphs { /* ... */ }
    namespace data_structures { /* ... */ }
}

namespace gr = algo::graphs;
namespace ds = algo::data_structures;
```

## 2. Repository Structure

| Path | What it is |
| --- | --- |
| `main.cpp` | The submission file: macros, aliases, the `algo` library, `main()`. |
| `docs/` | English documentation for every component. |
| `docs/graphs/` | Docs for `algo::graphs` — traversal, shortest paths, flows, trees. |
| `docs/structures/` | Docs for `algo::data_structures` — trees, queries, containers. |
| `benchmarks/` | Deterministic performance tests and their measured results. |
| `LICENSE` | MIT. |

## 3. License

MIT — see [LICENSE](LICENSE).

## 4. AI Usage

The code in this repository was written with AI assistance (Claude Opus 5, Anthropic). This is library code prepared **before** contests, which Codeforces rules permit in the same way as any other code published publicly ahead of a round. Contest solutions themselves are written by hand, unaided, during the round.
