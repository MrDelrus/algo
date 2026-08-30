# CLAUDE.md

What this repository is and the rules its code follows. Process — who decides what, how work is reviewed, how tasks are handed over — is deliberately not here. If a private, untracked `MEMORY.md` is present, it holds that, and it takes precedence over this file.

## What this repo is

A single-file competitive programming library targeting Codeforces. `main.cpp` is what gets submitted; everything else exists to make `main.cpp` trustworthy.

## Layout

```
main.cpp                     submission file — template + algo library + solve/main
.clang-format                Google style, two-space indent, 100 columns
scripts/format.sh            formats library code only
scripts/extract_library.sh   lifts namespace algo out of main.cpp into a header
docs/README.md               the index of every component — the only index
docs/structures/             one page per data structure
docs/graphs/                 one page per graph algorithm
tests/harness.hpp            shared assertion helpers
tests/structures/            correctness tests, one file per component
tests/graphs/
.github/workflows/ci.yml     build, tests, formatting, style rules
MEMORY.md                    untracked, personal
```

`docs/README.md` is the single index. The root `README.md` points at it and stays short; there are no per-area index files, because three indexes drift apart by the third component.

## Namespaces

The library lives in `namespace algo`, split into `algo::graphs` and `algo::data_structures`. Every component belongs to one of those two.

Inside an area, a component family gets its own namespace, named in the plural: `segment_trees` holds `segment_tree` and `lazy_segment_tree`. Namespaces never carry a leading underscore — that marks a private member, and a family namespace is not private, it is what a caller types to reach the core with a custom parameter. Genuinely internal helpers go in a nested `detail` namespace instead.

Shared vocabulary — monoids and anything else several families consume — stays one level up, directly in the area namespace, so a single type serves every structure that takes it.

**Aliases are declared last**, after every family namespace in the area is closed, gathered in one block. That block is the list of what the library actually offers, and keeping it in one place is the point.

Once components exist, short aliases are declared in `main.cpp` below the library block with `namespace gr = algo::graphs;` and `namespace ds = algo::data_structures;` — namespace aliases need `namespace`, not `using`.

## Code conventions

- **Language**: C++20 (Codeforces `GNU G++20 13.2 (64 bit, winlibs)`).
- **Repository language is English** — code, comments, docs, commit messages.
- **Self-contained**: a component must compile after copy-pasting it plus its declared dependencies. Dependencies are stated explicitly in the doc page.
- **Formatting**: clang-format, Google style, two-space indent, via `scripts/format.sh`. The tracked `.clang-format` is authoritative.
- **`snake_case` for everything**: classes, methods, free functions, namespaces, variables, constants. No PascalCase in the library — it reads like std. Private members carry a **leading** underscore (`_data`, `_size`). Template parameters are descriptive and lowercase (`value_type`, `operation`), never single letters.
- **No abbreviations, anywhere**: `segment_tree` not `seg_tree`, `fenwick` not `bit`, `left`/`right` not `l`/`r`. Applies to types, methods, parameters, and locals alike.
- **The exemption list is closed, and extending it is the user's call.** Two entries. First, the `main.cpp` competitive template — `all`, `pb`, `eb`, `fast`, `ll` — which is round shorthand, not library code. Second, the operation suffix in an alias name — `sum`, `min`, `max`, `gcd` — as in `segment_tree_min`, and only in type names.
- **Aliases put the operation last**: `segment_tree_min`, not `min_segment_tree`. Everything for one structure then sorts together.
- **Built-in integer types are banned in the library**: no `int`, no `long long`, no `unsigned`. Use `std::int64_t` by default, `std::int32_t` only where the bound is proven and memory matters, `std::size_t` for sizes, `std::uint64_t` for hashing and bit manipulation.
- **0-indexed** everywhere unless a structure is inherently 1-indexed (Fenwick internals); the public API stays 0-indexed regardless.
- **Comments are for invariants and complexity**, not for restating code. Each public component carries a one-line header comment: what it does plus its complexities.
- **No `using namespace std;` inside `namespace algo`** — the library must survive being pasted anywhere. `main.cpp`'s top-level template already has it.
- Prefer flat `std::vector` storage and indices over pointer-based nodes. No RTTI, no virtual dispatch.
- **Structures validate their arguments and throw** `std::out_of_range` or `std::invalid_argument` on misuse. A satisfied check is one predicted branch; the message is built only on the failing path. Debugging an index bug at speed is worth far more than the branch costs.
- Correctness first, then constant factor. A correctness invariant is never traded for speed without the trade being written down in the doc page's Notes.

## Markdown style

Paragraphs are single lines. Never hard-wrap prose at a column — let the editor wrap it.

## Documentation

Every component has one Markdown page under `docs/graphs/` or `docs/structures/`, named exactly after the component — `segment_tree.md`, matching the type it documents.

**A page exists so the API can be recovered a year later, by someone who already knows how the structure works.** It answers "what did we call this and what does it do", not "what is a segment tree". So: what each operation does in a line, the conventions it obeys — index base, half-open ranges, what a boolean return means — what it throws, and the traps that bite at the call site. No complexity tables, no explanation of the algorithm, no reasoning about design.

Sections: **Summary**, the API as tables of operations, **Traps**, **Related**. Forty to sixty lines is normal; past a hundred, something has leaked in that belongs elsewhere.

Rationale — why this structure, what was measured, what was rejected — goes to `docs/README.md` if it is worth keeping at all, and the argument behind a decision lives in the pull request that made it.

The same restraint applies to comments in `main.cpp`. A component gets two or three lines: what it does, the conventions it obeys, what it throws. Not the reasoning.

A component without a doc page is unfinished. Changing a component means updating its page in the same pass, and adding one means adding its row to `docs/README.md`.

## Speed

Complexity is the contract, and it is stated on every doc page. Measurements are not kept in the repository: timing gates were tried and removed because maintaining them cost more attention than the regressions they caught were worth.

One consequence to be honest about — **replacing an std component is on hold.** That decision needed a measured 2x win, and there is nothing left to measure with. A rewrite of something `std` already provides waits until benchmarking comes back.

## Tests

`tests/` mirrors `docs/` too. Run them with `scripts/run_tests.sh`; everything compiles with ASan and UBSan. What a test file owes is in `tests/README.md` — the short version: check against an obvious reference, be exhaustive for `n <= 32`, hit the power-of-two boundaries, include a non-commutative monoid, cover what must throw *and* what must not, and carry a couple of deterministic cases at `n` near 1e3 verified against a quadratic reference.

A component is unfinished without tests, exactly as it is unfinished without a doc page.

## Build

```bash
g++ -std=c++20 -O2 -Wall -Wextra -o /tmp/main main.cpp
g++ -std=c++20 -g -fsanitize=address,undefined -o /tmp/main_dbg main.cpp
```

## Formatting boundary in main.cpp

Formatting rules apply **only inside `namespace algo`**. The competitive template around it — includes, macros, aliases, `solve()`, `main()` — is hand-arranged and stays exactly as written; the blank lines inside `solve()` are deliberate typing room, not slop to clean up.

Never run bare `clang-format` on `main.cpp` — it would reformat the template. Run `scripts/format.sh` instead: it locates the namespace body by the lines `namespace algo {` and `}  // namespace algo` and passes only that line range to clang-format, then formats the tests whole. Both those lines are load-bearing and must stay exactly as they are — the formatter and the CI style check key off them, and the script fails loudly if either is missing or duplicated.

## CI

`.github/workflows/ci.yml` runs on every push and pull request, and must stay green:

- **build** — `main.cpp` with `-Werror`, plus an ASan/UBSan build and a smoke run.
- **clang-format** — `scripts/format.sh --check`.
- **style rules** — extracts the `namespace algo { ... }` block from `main.cpp` and greps it, plus the tests, for built-in integer types.
