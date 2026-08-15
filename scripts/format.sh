#!/usr/bin/env bash
# Formats library code only.
#
# In main.cpp the competitive template (includes, macros, aliases, solve, main) is
# hand-arranged and must stay untouched, so only the body of `namespace algo` is passed
# to clang-format. Benchmarks are formatted whole.
#
# Usage:
#   scripts/format.sh          rewrite files in place
#   scripts/format.sh --check  fail if anything is unformatted (used by CI)

set -euo pipefail

cd "$(dirname "$0")/.."

mode=(-i)
if [[ "${1:-}" == "--check" ]]; then
  mode=(--dry-run --Werror)
fi

opening="namespace algo {"
closing="}  // namespace algo"

count_lines() {
  grep -c -x -F "$1" main.cpp || true
}

if [[ "$(count_lines "$opening")" != "1" || "$(count_lines "$closing")" != "1" ]]; then
  echo "error: main.cpp must contain exactly one '$opening' line and one '$closing' line" >&2
  exit 1
fi

first=$(($(grep -n -x -F "$opening" main.cpp | cut -d: -f1) + 1))
last=$(($(grep -n -x -F "$closing" main.cpp | cut -d: -f1) - 1))

if ((last >= first)); then
  clang-format "${mode[@]}" --lines="$first:$last" main.cpp
else
  echo "library region is empty, nothing to format in main.cpp"
fi

shopt -s nullglob
benchmarks=(benchmarks/*.cpp)
if ((${#benchmarks[@]} > 0)); then
  clang-format "${mode[@]}" "${benchmarks[@]}"
fi
