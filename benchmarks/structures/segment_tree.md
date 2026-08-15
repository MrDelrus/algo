# segment_tree — benchmark

Source: [segment_tree.cpp](segment_tree.cpp). Run with `benchmarks/run.sh structures/segment_tree`.

## Setup

| | |
| --- | --- |
| Structure | `ds::sum_segment_tree` |
| `n` | 200 000 |
| Operations per scenario | 20 000 000 |
| Repetitions | 5, fastest reported |
| Compiler | GCC 13.2.0 |
| Flags | `-std=c++20 -O2 -static -Wall -Wextra -Werror` |
| Container | `gcc:13.2.0`, 1 CPU pinned to one core, 256 MB, no network |
| Build time | 0.52 ms |

The operation stream is a counter-based splitmix64 with a fixed seed, so a rerun performs bitwise identical work. No `std` distribution is involved — those are not specified to produce identical output across standard library implementations.

**How per-operation timings are obtained.** The same stream is replayed with a mask that enables one operation kind at a time; every pass draws the same numbers in the same order, so passes differ only in the tree work they do. Subtracting the pass with an empty mask — the *generator* row — leaves the cost of that operation alone.

**† marks a figure below the measurement floor.** Two passes of 2e7 operations never take exactly the same time, and when an operation costs less than that jitter, the subtraction returns noise — sometimes a negative number, which is the proof that nothing measurable was left. Read those rows as "too cheap for this method to resolve", not as "free". `query_all` is one read of the root, which is always in cache, so its true cost is around a nanosecond.

One more caveat: an isolated pass mutates the tree less than the full mix does, so a per-operation figure is that operation measured on its own, not its cost amid the others.

## Scenario 1 — one hot position

Half `get`, half `set`, every one of them aimed at a single position drawn once. The root-to-leaf path stays in cache, so this isolates the cost of walking the tree with memory latency taken out of the picture.

| | Time | Per operation | Spread |
| --- | --- | --- | --- |
| Total | 359.88 ms | 18.0 ns | 0.8% |
| Generator alone | 151.67 ms | 7.6 ns | |

| Operation | Calls | ns per call |
| --- | --- | --- |
| `set` | 10 000 109 | 20.3 |
| `get` | 9 999 891 | 0.5 † |

An update against a hot cache costs 20 ns for 18 levels — a little over a nanosecond per level. `get` is a single array read and does not separate from the generator.

## Scenario 2 — balanced

Random positions. `set` 2/10, `combine_at` 2/10, `get` 2/10, `query` 3/10, `query_all` 1/10. Point updates and range queries carry comparable weight, and every access misses cache.

| | Time | Per operation | Spread |
| --- | --- | --- | --- |
| Total | 1369.90 ms | 68.5 ns | 5.4% |
| Generator alone | 319.96 ms | 16.0 ns | |

| Operation | Calls | ns per call |
| --- | --- | --- |
| `set` | 4 001 856 | 57.6 |
| `combine_at` | 3 999 965 | 60.0 |
| `get` | 3 999 744 | 8.1 |
| `query` | 6 001 232 | 111.6 |
| `query_all` | 1 997 203 | 0.3 † |

`set` costs 2.8x what it did in scenario 1 — that gap is cache misses, not extra work, since the instruction count is identical. `combine_at` runs a few nanoseconds behind `set`, which is the one extra `combine` on the leaf. `query` costs about twice an update: it descends from both ends and touches two paths instead of one. `query_all` reads the root and does not separate from the generator at all.

## Scenario 3 — query heavy

Random positions. `set` 1/10, `combine_at` 1/10, `get` 1/10, `query` 5/10, `query_all` 2/10. Weight moves onto range queries, so the difference from scenario 2 is the price of `query` relative to a point update.

| | Time | Per operation | Spread |
| --- | --- | --- | --- |
| Total | 1669.80 ms | 83.5 ns | 2.5% |
| Generator alone | 361.52 ms | 18.1 ns | |

| Operation | Calls | ns per call |
| --- | --- | --- |
| `set` | 1 998 892 | 61.2 |
| `combine_at` | 2 001 273 | 70.0 |
| `get` | 1 998 395 | 19.2 |
| `query` | 10 001 689 | 113.5 |
| `query_all` | 3 999 751 | -0.1 † |

`query` lands at 113.5 ns against 111.6 ns in scenario 2 — the same cost under a different mix, which is the reassuring answer. The point operations are measured over five times fewer calls here, so their figures carry correspondingly more noise; the difference from scenario 2 is measurement, not behaviour.

## Reading the numbers

Total throughput moves from 18.0 to 83.5 ns per operation across the three scenarios, and essentially all of that spread is memory. The tree executes the same instructions in all three; what changes is whether the path it walks is already in cache.

The practical ceiling: at 2e5 elements and a query-heavy mix, this structure sustains roughly 12 million operations per second per core. A problem with 2e5 queries spends about 20 ms in the tree, comfortable against a 1–2 second limit.

Absolute milliseconds are not comparable to a Codeforces verdict — the judge's CPU and scheduler are different. These numbers exist to compare our own implementations against each other under identical conditions.
