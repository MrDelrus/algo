# CLAUDE.md

What this repository is and the rules its code follows. Process — who decides what, how work is reviewed, how tasks are handed over — is deliberately not here. If a private, untracked `MEMORY.md` is present, it holds that, and it takes precedence over this file.

## What this repo is

A single-file competitive programming library targeting Codeforces. `main.cpp` is what gets submitted; everything else exists to make `main.cpp` trustworthy.

## Layout

```
main.cpp                  submission file — template + algo library + solve/main
.clang-format             Google style, two-space indent, 100 columns
scripts/format.sh         formats library code only
docs/graphs/              docs for algo::graphs
docs/structures/          docs for algo::data_structures
benchmarks/               deterministic performance tests + results
.github/workflows/ci.yml  build, benchmarks, formatting, style rules
MEMORY.md                 untracked, personal
```

## Namespaces

The library lives in `namespace algo`, split into `algo::graphs` and `algo::data_structures`. Every component belongs to one of those two.

Once components exist, short aliases are declared in `main.cpp` below the library block with `namespace gr = algo::graphs;` and `namespace ds = algo::data_structures;` — namespace aliases need `namespace`, not `using`.

## Code conventions

- **Language**: C++20 (Codeforces `GNU G++20 13.2 (64 bit, winlibs)`).
- **Repository language is English** — code, comments, docs, commit messages.
- **Self-contained**: a component must compile after copy-pasting it plus its declared dependencies. Dependencies are stated explicitly in the doc page.
- **Formatting**: clang-format, Google style, two-space indent, via `scripts/format.sh`. The tracked `.clang-format` is authoritative.
- **`snake_case` for everything**: classes, methods, free functions, namespaces, variables, constants. No PascalCase in the library — it reads like std. Private members carry a **leading** underscore (`_data`, `_size`). Template parameters are descriptive and lowercase (`value_type`, `operation`), never single letters.
- **No abbreviations, anywhere**: `segment_tree` not `seg_tree`, `fenwick` not `bit`, `left`/`right` not `l`/`r`. Applies to types, methods, parameters, and locals alike. The `main.cpp` competitive template (`all`, `pb`, `eb`, `fast`, `ll`) is the sole exemption — it is round shorthand, not library code.
- **Built-in integer types are banned in the library**: no `int`, no `long long`, no `unsigned`. Use `std::int64_t` by default, `std::int32_t` only where the bound is proven and memory matters, `std::size_t` for sizes, `std::uint64_t` for hashing and bit manipulation.
- **0-indexed** everywhere unless a structure is inherently 1-indexed (Fenwick internals); the public API stays 0-indexed regardless.
- **Comments are for invariants and complexity**, not for restating code. Each public component carries a one-line header comment: what it does plus its complexities.
- **No `using namespace std;` inside `namespace algo`** — the library must survive being pasted anywhere. `main.cpp`'s top-level template already has it.
- Prefer flat `std::vector` storage and indices over pointer-based nodes. No exceptions, no RTTI, no virtual dispatch in hot paths.
- Correctness first, then constant factor. A correctness invariant is never traded for speed without the trade being written down in the doc page's Notes.

## Markdown style

Paragraphs are single lines. Never hard-wrap prose at a column — let the editor wrap it.

## Documentation

Every component has one Markdown page under `docs/graphs/` or `docs/structures/`, named after the component in `kebab-case.md`, with these sections:

1. **Summary** — one sentence.
2. **Complexity** — build / query / update, time and memory.
3. **API** — signatures with parameter semantics and index conventions.
4. **Usage** — a short, real snippet.
5. **Notes** — invariants, precision limits, overflow risks, when *not* to use it.
6. **Related** — links to sibling pages.

A component without a doc page is unfinished. Changing a component means updating its page in the same pass, and the `docs/<area>/README.md` index table stays in sync.

## Benchmarks

Rules live in `benchmarks/README.md`. The load-bearing ones: benchmarks are fully deterministic (no `rng`, no `chrono` seeding, fixed-seed data generation), `n = 2e5` is the standard size for O(n log n) structures, query results are accumulated into a printed checksum so the optimizer cannot delete the measured work, and results are always reported with machine, compiler, and flags.

An std component is only replaced by a hand-written one when a benchmark shows at least a 2x win.

## Build

```bash
g++ -std=c++20 -O2 -Wall -Wextra -o /tmp/main main.cpp
g++ -std=c++20 -g -fsanitize=address,undefined -o /tmp/main_dbg main.cpp
```

## Formatting boundary in main.cpp

Formatting rules apply **only inside `namespace algo`**. The competitive template around it — includes, macros, aliases, `solve()`, `main()` — is hand-arranged and stays exactly as written; the blank lines inside `solve()` are deliberate typing room, not slop to clean up.

Never run bare `clang-format` on `main.cpp` — it would reformat the template. Run `scripts/format.sh` instead: it locates the namespace body by the lines `namespace algo {` and `}  // namespace algo` and passes only that line range to clang-format, then formats the benchmarks whole. Both those lines are load-bearing and must stay exactly as they are — the formatter and the CI style check key off them, and the script fails loudly if either is missing or duplicated.

## CI

`.github/workflows/ci.yml` runs on every push and pull request, and must stay green:

- **build** — `main.cpp` with `-Werror`, plus an ASan/UBSan build and a smoke run.
- **benchmarks** — every `benchmarks/*.cpp` compiles with `-Werror`.
- **clang-format** — `scripts/format.sh --check`.
- **style rules** — extracts the `namespace algo { ... }` block from `main.cpp` and greps it, plus the benchmarks, for built-in integer types.
