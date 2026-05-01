# Matching Engine

## Overview

Minimal C++ matching engine project scaffold with CMake, GoogleTest, and Google Benchmark integration.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run tests

```bash
ctest --test-dir build --output-on-failure
```

## Run benchmarks

Benchmark targets are built via CMake with Google Benchmark available in the project.  
Add and run benchmark executables as they are introduced.

Throughput benchmark:

```bash
./build/order_book_benchmark --benchmark_min_time=0.05s --benchmark_format=json --benchmark_out=benchmarks/baseline_benchmark.json
```

Latency percentile benchmark (steady-state, per-op samples):

```bash
./build/order_book_latency_benchmark --ops=1000000 --out=benchmarks/latency_percentiles.md
```

### Benchmark hygiene

- Use Release builds (`-O3 -DNDEBUG`) and keep sanitizers off for perf runs.
- Record machine details with results (CPU model, core count, memory).
- Pin benchmark runs to dedicated cores when your OS supports it.
  - Linux example: `taskset -c 2 ./build/order_book_latency_benchmark ...`
- Keep workload shape explicit and stable (price-level distribution, match rate, and cancel mix).
- Include both throughput and latency percentiles (`p50`, `p90`, `p99`, `p99.9`, `max`) in reporting.

## Architecture

## Design decisions

## Benchmarks

## Roadmap

## References
