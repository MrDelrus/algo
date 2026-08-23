#!/usr/bin/env bash
# Builds and runs every correctness test.
#
# Tests are compiled with ASan and UBSan: a segment tree that reads one element past its buffer
# still returns the right answer most of the time, and only a sanitizer makes that visible.
#
# Usage:
#   scripts/run_tests.sh                            every test
#   scripts/run_tests.sh structures/segment_tree    one of them

set -euo pipefail

cd "$(dirname "$0")/.."

generated="tests/generated"
mkdir -p "$generated"
scripts/extract_library.sh "$generated/algo_library.hpp" > /dev/null

shopt -s nullglob

if [[ $# -gt 0 ]]; then
  names=("$@")
else
  names=()
  for source in tests/*/*.cpp; do
    name="${source#tests/}"
    names+=("${name%.cpp}")
  done
fi

if [[ ${#names[@]} -eq 0 ]]; then
  echo "no tests yet"
  exit 0
fi

failed=0

for name in "${names[@]}"; do
  source="tests/$name.cpp"
  if [[ ! -f "$source" ]]; then
    echo "error: no such test: $source" >&2
    exit 1
  fi

  binary="/tmp/algo-test-$(basename "$name")"
  echo "=== $name ==="
  g++ -std=c++20 -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all \
    -I "$generated" -o "$binary" "$source"
  if ! "$binary"; then
    failed=1
  fi
  echo
done

if [[ $failed -ne 0 ]]; then
  echo "TESTS FAILED"
  exit 1
fi

echo "all tests passed"
