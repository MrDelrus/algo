# CLAUDE.md

Working notes for this repository. Read `MEMORY.md` (untracked) first — it holds the collaboration rules that override anything here.

## What this repo is

A single-file competitive programming library targeting Codeforces. `main.cpp` is what gets submitted; everything else exists to make `main.cpp` trustworthy.

## Layout

```
main.cpp            submission file — template + algo library + main()
docs/graphs/        docs for algo::graphs
docs/structures/    docs for algo::data_structures
benchmarks/         deterministic perf tests + results
MEMORY.md           untracked, collaboration rules
```

## Namespaces

```
algo::graphs           -> alias gr
algo::data_structures  -> alias ds
```

Namespace aliases are declared with `namespace gr = algo::graphs;` (not `using`) once in `main.cpp` after the library block. New components go into one of the two existing subnamespaces; do not invent a third without asking.

## Code conventions

- **Language**: C++20 (Codeforces `GNU G++20 13.2 (64 bit, winlibs)`).
- **Repository language is English** — code, comments, docs, commit messages. No Russian in tracked files.
- **Self-contained**: a component must compile after copy-pasting it plus its declared dependencies. State dependencies explicitly in the doc page.
- **Formatting**: clang-format, Google style, two-space indent. The tracked `.clang-format` is authoritative — run `clang-format -i` on anything you touch.
- **`snake_case` for everything**: classes, methods, free functions, namespaces, variables, constants. No PascalCase in the library — it should read like std. Private members carry a **leading** underscore (`_data`, `_size`). Template parameters are descriptive and lowercase (`value_type`, `operation`), never single letters.
- **No abbreviations, anywhere**: `segment_tree` not `seg_tree`, `fenwick` not `bit`, `left`/`right` not `l`/`r`. Applies to types, methods, parameters, and locals alike. The `main.cpp` competitive template (`pb`, `all`, `dbg`) is the sole exemption — it is round shorthand, not library code.
- **Built-in integer types are banned in the library**: no `int`, no `long long`, no `unsigned`. Use `std::int64_t` by default, `std::int32_t` only where the bound is proven and memory matters, `std::size_t` for sizes, `std::uint64_t` for hashing and bit manipulation.
- **0-indexed** everywhere unless a structure is inherently 1-indexed (Fenwick internals); the public API stays 0-indexed regardless.
- **Comments are for invariants and complexity**, not for restating code. Each public component carries a one-line header comment: what it does + complexities.
- **No `using namespace std;` inside `namespace algo`** — the library must survive being pasted anywhere. `main.cpp`'s top-level template already has it.
- Prefer flat `std::vector` storage and indices over pointer-based nodes. No exceptions, no RTTI, no virtual dispatch in hot paths.

## Markdown style

Paragraphs are single lines. Never hard-wrap prose at a column — let the editor wrap it.

## Documentation

Every component gets one Markdown page under `docs/graphs/` or `docs/structures/`, named after the component in `kebab-case.md`. Each page has:

1. **Summary** — one sentence.
2. **Complexity** — build / query / update, time and memory.
3. **API** — signatures with parameter semantics and index conventions.
4. **Usage** — a short, real snippet.
5. **Notes** — invariants, precision limits, overflow risks, when *not* to use it.
6. **Related** — links to sibling pages.

Keep `docs/<area>/README.md` index tables in sync whenever a component is added.

## Benchmarks

The user designs the benchmark scenario for each structure; my job is to turn it into one concrete, fully deterministic test and report the numbers. Rules are in `benchmarks/README.md` — the important ones: no `rng`/`chrono` seeding in a benchmark, `n = 2e5` for O(n log n) structures, and results are always reported with machine, compiler, and flags.

## Component workflow

Components are added in a fixed order, never out of order: the user picks the component, we agree the API, we agree the benchmark, I implement and verify, the user reviews. Do not write an implementation while the API is still open.

**One component, one pull request**, on its own branch, containing exactly that component's implementation, doc page (plus the area index row), benchmark, and the measured results — nothing unrelated. Report the numbers in both the PR description and the doc page. Never push to `main` directly and never merge a pull request; the user reviews and merges by hand. CI must be green before review is requested.

## Editing rules

- Changing a component means updating its doc page in the same pass. A component without docs is unfinished.
- Do not reformat or restructure unrelated parts of `main.cpp` while adding something.
- Correctness first, then constant factor. Never trade a correctness invariant for speed without saying so in the doc page's Notes.
- When a design has real trade-offs (memory vs. speed, generality vs. constant factor), present the options and let the user pick rather than silently choosing.

## Build

```bash
g++ -std=c++20 -O2 -Wall -Wextra -o /tmp/main main.cpp
g++ -std=c++20 -g -fsanitize=address,undefined -o /tmp/main_dbg main.cpp
```

## CI

`.github/workflows/ci.yml` runs on every push and pull request, and must stay green:

- **build** — `main.cpp` with `-Werror`, plus an ASan/UBSan build and a smoke run.
- **benchmarks** — every `benchmarks/*.cpp` compiles with `-Werror`.
- **clang-format** — `scripts/format.sh --check`.
- **style rules** — extracts the `namespace algo { ... }` block from `main.cpp` and greps it, plus the benchmarks, for built-in integer types.

## Formatting boundary in main.cpp

Formatting rules apply **only inside `namespace algo`**. The competitive template around it — includes, macros, aliases, `solve()`, `main()` — is hand-arranged and must stay exactly as written; the blank lines inside `solve()` are deliberate typing room, not slop to clean up.

Never run bare `clang-format` on `main.cpp` — it would reformat the template. Run `scripts/format.sh` instead: it locates the namespace body by the lines `namespace algo {` and `}  // namespace algo` and passes only that line range to clang-format, then formats the benchmarks whole. Both those lines must stay exactly as they are — the formatter and the CI style check both key off them, and the script fails loudly if either is missing or duplicated.

## Reference material

`Template_for_CodeForces/` is a cloned third-party template kept locally for reference. It is not part of this library, its structures have known defects, and nothing from it is copied without being rewritten and benchmarked.
