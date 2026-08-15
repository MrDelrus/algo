# Benchmarks

Speed only — correctness belongs to `tests/`.

**Results live in the component's documentation page**, under a Performance section, not here. This directory holds the measuring code and the environment it runs in; a component has one page and its numbers belong on it.

## Layout

Mirrors `docs/`: one directory per area, one `.cpp` per component.

```
benchmarks/structures/segment_tree.cpp
benchmarks/graphs/
```

## Running

```bash
benchmarks/run.sh                            every benchmark
benchmarks/run.sh structures/segment_tree    one of them
```

`run.sh` regenerates the library header from `main.cpp`, builds the image, and runs in a container with one pinned CPU, 256 MB, and no network. Override with `BENCHMARK_CPU_CORE` and `BENCHMARK_MEMORY`.

Benchmarks cannot include `main.cpp` — it defines `main()`. `scripts/extract_library.sh` lifts `namespace algo` into `benchmarks/generated/algo_library.hpp`, which is gitignored and regenerated on every run, so a benchmark can never measure a stale copy.

The container pins GCC 13.2.0 and `-std=c++20 -O2 -static`, matching Codeforces. It cannot reproduce the judge's CPU or scheduler, so **absolute times are not comparable to a verdict** — they compare our implementations against each other.

## Rules

- **One number per scenario: the total.** Do not split a total across operation kinds. The jitter between two passes is a few percent, and dividing it by the calls of a minority operation leaves an uncertainty larger than the operation being measured. What one operation costs is what its complexity says it costs.
- Deterministic: fixed-seed counter-based splitmix64, no `rng`, no `chrono` seeding, no `std` distributions (they differ between standard library implementations).
- `n = 2e5` for O(log n) operations; other complexity classes pick a size in the same time band and say so.
- Measure a pass that draws the identical stream but touches nothing, and report it beside the total — the generator is not free.
- Accumulate results into a printed checksum, so the optimizer keeps the work and matching checksums prove the run was deterministic.
- One untimed warm-up pass on a smaller instance.
- Report the fastest repetition and the spread across them.
- A hand-written replacement for something in `std` is kept only on a measured 2x win.
