# segment_tree — benchmark

Source: [segment_tree.cpp](segment_tree.cpp). Run with `benchmarks/run.sh structures/segment_tree`.

## Setup

| | |
| --- | --- |
| Structure | `segment_tree<sum_monoid<std::int64_t>>` |
| `n` | 200 000 |
| Operations per scenario | 20 000 000 |
| Repetitions | 5, fastest reported |
| Compiler | GCC 13.2.0 |
| Flags | `-std=c++20 -O2 -static -Wall -Wextra -Werror` |
| Container | `gcc:13.2.0`, 1 CPU pinned to one core, 256 MB, no network |
| Build time | 0.53 ms |

The operation stream is a counter-based splitmix64 with a fixed seed, so a rerun performs bitwise identical work. No `std` distribution is involved — those are not specified to produce identical output across standard library implementations.

**How per-operation timings are obtained.** The same stream is replayed with a mask that enables one operation kind at a time; every pass draws the same numbers in the same order, so passes differ only in the tree work they do. Subtracting the pass with an empty mask — the *generator* row — leaves the cost of that operation alone.

Two consequences worth knowing. Costs under roughly one nanosecond sit inside the measurement floor and should be read as "too cheap to separate from the generator", not as zero. And an isolated pass mutates the tree less than the full mix does, so a per-operation figure is that operation measured on its own, not its cost amid the others.

## Scenario 1 — one hot position

Half `get`, half `set`, every one of them aimed at a single position drawn once. The root-to-leaf path stays in cache, so this isolates the cost of walking the tree with memory latency taken out of the picture.

| | Time | Per operation | Spread |
| --- | --- | --- | --- |
| Total | 360.15 ms | 18.0 ns | 0.8% |
| Generator alone | 154.18 ms | 7.7 ns | |

| Operation | Calls | ns per call |
| --- | --- | --- |
| `set` | 10 000 109 | 20.0 |
| `get` | 9 999 891 | below the floor |

An update against a hot cache costs 20 ns for 18 levels — a little over a nanosecond per level. `get` is a single array read and does not separate from the generator.

## Scenario 2 — balanced

Random positions. `set` 2/10, `combine_at` 2/10, `get` 2/10, `query` 3/10, `query_all` 1/10. Point updates and range queries carry comparable weight, and every access misses cache.

| | Time | Per operation | Spread |
| --- | --- | --- | --- |
| Total | 1316.84 ms | 65.8 ns | 9.0% |
| Generator alone | 318.72 ms | 15.9 ns | |

| Operation | Calls | ns per call |
| --- | --- | --- |
| `set` | 4 001 856 | 53.3 |
| `combine_at` | 3 999 965 | 55.3 |
| `get` | 3 999 744 | 3.9 |
| `query` | 6 001 232 | 108.4 |
| `query_all` | 1 997 203 | below the floor |

`set` costs 2.7x what it did in scenario 1 — that gap is cache misses, not extra work, since the instruction count is identical. `combine_at` runs 2 ns behind `set`, which is the one extra `combine` on the leaf. `query` costs about twice an update: it descends from both ends and touches two paths instead of one. `query_all` reads the root and disappears into the noise.

## Scenario 3 — query heavy

Random positions. `set` 1/10, `combine_at` 1/10, `get` 1/10, `query` 5/10, `query_all` 2/10. Weight moves onto range queries, so the difference from scenario 2 is the price of `query` relative to a point update.

| | Time | Per operation | Spread |
| --- | --- | --- | --- |
| Total | 1697.69 ms | 84.9 ns | 4.2% |
| Generator alone | 359.93 ms | 18.0 ns | |

| Operation | Calls | ns per call |
| --- | --- | --- |
| `set` | 1 998 892 | 72.2 |
| `combine_at` | 2 001 273 | 73.3 |
| `get` | 1 998 395 | 17.4 |
| `query` | 10 001 689 | 109.5 |
| `query_all` | 3 999 751 | below the floor |

`query` lands at 109.5 ns against 108.4 ns in scenario 2 — the same cost under a different mix, which is the reassuring answer. The point operations look more expensive here only because they are rarer: with fewer updates their cache lines are colder by the time they are touched again.

## Reading the numbers

Total throughput moves from 18 to 85 ns per operation across the three scenarios, and every bit of that spread is memory. The tree does the same instructions in all three; what changes is whether the path it walks is already in cache.

The practical ceiling: at 2e5 elements and a query-heavy mix, this structure sustains roughly 12 million operations per second per core. A problem with 2e5 queries spends about 20 ms in the tree, which is comfortable against a 1–2 second limit.

Absolute milliseconds are not comparable to a Codeforces verdict — the judge's CPU and scheduler are different. These numbers exist to compare our own implementations against each other under identical conditions.
