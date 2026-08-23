#!/usr/bin/env bash
# Runs a benchmark inside a container with fixed resources.
#
# Usage:
#   benchmarks/run.sh                 run every benchmark
#   benchmarks/run.sh segment_tree    run one, by file name without the extension
#
# The container gets one CPU, pinned to a single core so the process is never migrated, and
# 256 MB — the memory a Codeforces problem usually grants. Networking is off. The library
# header is regenerated from main.cpp before every run, so a benchmark can never measure a
# stale copy of the code.

set -euo pipefail

cd "$(dirname "$0")/.."

image="algo-benchmarks"
cpu_core="${BENCHMARK_CPU_CORE:-0}"
memory="${BENCHMARK_MEMORY:-256m}"

generated="benchmarks/generated"
mkdir -p "$generated"
scripts/extract_library.sh "$generated/algo_library.hpp" > /dev/null

if [[ $# -gt 0 ]]; then
  names=("$@")
else
  names=()
  shopt -s nullglob
  for source in benchmarks/*/*.cpp; do
    names+=("${source#benchmarks/}")
    names[-1]="${names[-1]%.cpp}"
  done
fi

if [[ ${#names[@]} -eq 0 ]]; then
  echo "no benchmarks to run"
  exit 0
fi

echo "building image $image"
docker build --quiet --tag "$image" --file benchmarks/Dockerfile benchmarks > /dev/null

for name in "${names[@]}"; do
  source="benchmarks/$name.cpp"
  binary="$(basename "$name")"
  if [[ ! -f "$source" ]]; then
    echo "error: no such benchmark: $source" >&2
    exit 1
  fi

  echo
  echo "=== $name ==="
  docker run --rm \
    --cpus=1 \
    --cpuset-cpus="$cpu_core" \
    --memory="$memory" \
    --network=none \
    --volume "$PWD:/benchmark:ro" \
    --workdir /benchmark \
    "$image" \
    bash -c "
      set -euo pipefail
      g++ \$BENCHMARK_FLAGS -I benchmarks/generated -o /tmp/$binary $source
      echo \"compiler: \$(g++ --version | head -1)\"
      echo \"flags:    \$BENCHMARK_FLAGS\"
      echo \"cpu:      \$(nproc) core(s) visible, pinned to core $cpu_core\"
      echo \"memory:   $memory\"
      echo
      /tmp/$binary
    "
done
