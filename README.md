# algo

Pre-written C++20 algorithms and data structures for Codeforces rounds.

## Overview

A personal library written ahead of time (with the help of Claude Opus 5) and published, so it can be reused during rounds. `main.cpp` is the whole thing: a competitive template, the library inside `namespace algo`, and a `solve()` to fill in. Nothing here is round-specific — it is the reusable half of a solution, the half that should never need debugging under contest pressure.

## Repository structure

| Path | What it is |
| --- | --- |
| `main.cpp` | The submission file. |
| [docs/](docs/) | **Index of every component and its documentation.** |
| [tests/](tests/) | Correctness tests, run under sanitizers. |
| `scripts/` | Formatting, library extraction, the test runner. |

## License

MIT — see [LICENSE](LICENSE).

## AI usage

The code here was written with AI assistance (Claude Opus 5, Anthropic). It is library code prepared **before** contests, which Codeforces rules permit in the same way as any other code published publicly ahead of a round. Contest solutions themselves are written by hand, unaided, during the round.
