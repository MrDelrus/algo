# Benchmark environment, kept as close to a Codeforces invoker as a container can get.
#
# Codeforces compiles GNU G++20 with GCC 13.2, runs a single-threaded process, and grants it
# 256 MB. The compiler version is pinned here; the CPU and memory limits are applied by
# run.sh at container start, since they are runtime properties rather than image properties.
#
# What a container cannot reproduce: the judge's exact CPU model, its clock behaviour, and
# Windows scheduling. Absolute milliseconds are therefore not comparable to a Codeforces
# verdict — the numbers are for comparing our own implementations against each other under
# identical conditions.

FROM gcc:13.2.0

ENV LANG=C.UTF-8

# Codeforces builds with -O2 and links statically. Matching that removes dynamic loader noise
# from the measurement as a bonus.
ENV BENCHMARK_FLAGS="-std=c++20 -O2 -static -Wall -Wextra -Werror"

WORKDIR /benchmark
