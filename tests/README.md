# Tests

Correctness. Speed is reasoned about as complexity and not measured here.

## Layout

Mirrors `docs/`: one directory per area, one file per component, plus a shared `harness.hpp`.

```
tests/harness.hpp
tests/structures/segment_tree.cpp
tests/graphs/
```

## Running

```bash
scripts/run_tests.sh                            every test
scripts/run_tests.sh structures/segment_tree    one of them
```

Everything is compiled with ASan and UBSan and `-fno-sanitize-recover=all`. A structure that reads one element past its buffer usually still returns the right answer, and only a sanitizer makes that visible.

Tests include the library through the header `scripts/extract_library.sh` generates from `main.cpp`, so they always run against the real code rather than a copy that would drift.

## What a test file must do

- **Check against an obvious reference, never against itself.** A linear fold, an O(n²) scan — something so simple it is clearly right.
- **Be exhaustive where it is cheap.** For `n <= 32`: every size, every range, every position, every starting point. Most bugs are off-by-one, and an off-by-one has nowhere to hide in an exhaustive sweep.
- **Hit the shape boundaries.** Sizes at and around powers of two, `n = 0`, `n = 1`, the first and last element, the empty range at the very end.
- **Include a non-commutative case.** String concatenation turns a reversed fold into a reversed string; sum, min and max would hide it.
- **Carry a couple of large deterministic cases**, `n` around 1e3, generated from a fixed seed and verified against the quadratic reference.
- **Be deterministic.** Counter-based splitmix64 with a fixed seed, no `std` distributions. A failure must reproduce byte for byte on any machine.
- **Say where it failed.** Every check carries the size and the indices involved, so a failure names the case without a debugger.

A failing test that turns out to be wrong about the structure is still a finding: the preconditions it violated belong in the documentation.
