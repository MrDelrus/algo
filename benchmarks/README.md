# Benchmarks

Timing gates, not reports. Each one runs a structure under a fixed workload and **fails the build** when it no longer fits its time budget. Correctness belongs to `tests/`; speed belongs here, and nowhere else — no numbers are published in the documentation, because a recorded number goes stale the moment nobody re-measures it, while a gate cannot.

## Layout

Mirrors `docs/` and `tests/`: one directory per area, one `.cpp` per component.

```
benchmarks/structures/segment_tree.cpp
benchmarks/graphs/
```

## Running

```bash
scripts/extract_library.sh benchmarks/generated/algo_library.hpp
g++ -std=c++20 -O2 -Wall -Wextra -Werror -I benchmarks/generated -o /tmp/bench benchmarks/structures/segment_tree.cpp
/tmp/bench
```

or, pinned to one core with the Codeforces compiler in a container:

```bash
benchmarks/run.sh structures/segment_tree
```

Benchmarks cannot include `main.cpp` — it defines `main()`. `scripts/extract_library.sh` lifts `namespace algo` into `benchmarks/generated/algo_library.hpp`, which is gitignored and regenerated on every run, so a benchmark can never measure a stale copy.

## In CI

The `benchmarks` job runs only what the diff touched: a change to `main.cpp` affects every structure and runs all of them, a change to one benchmark runs only that one, and a diff touching neither skips the job. Timing every structure on every documentation typo would burn minutes and invite flaky reds.

## Rules

- **A budget, and the measured time.** The verdict is what CI acts on, but the number is printed so a slow slide from 15 ms toward the limit is visible long before it crosses.
- **`n = 2e5` with 2e5 operations, budget 100 ms** for structures whose operations are O(log n). Measured cost today is 9–16 ms, so the budget carries roughly a sevenfold margin — deliberately. A shared runner is slower and noisier than a desktop, while the regression worth catching, an O(log n) operation quietly becoming O(n), overshoots any budget by orders of magnitude rather than by percent.
- **Deterministic.** Fixed-seed counter-based splitmix64. No `rng`, no `chrono` seeding, no `std` distributions — those are not specified to produce identical output across standard library implementations.
- **Report the fastest repetition.** Every repetition performs bitwise identical work, so differences between them are outside interference; holding the build to the unluckiest run makes the gate flaky rather than strict.
- **Consume the results.** Accumulate answers into a printed checksum, so the optimizer cannot delete the work being measured.
- **One untimed warm-up pass** on a smaller instance.
- **One component per file**, named after it.
- **Exit non-zero when over budget.** That is the whole point.
