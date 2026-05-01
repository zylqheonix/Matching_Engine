# Baseline Benchmark Numbers

Captured on 2026-05-01 with:

`./build/order_book_benchmark --benchmark_min_time=0.05s --benchmark_format=json --benchmark_out=benchmarks/baseline_benchmark.json`

Throughput highlights (`items_per_second`):

- `BM_AddLimitOrder/64`: 22.98 M ops/s
- `BM_AddLimitOrder/512`: 23.90 M ops/s
- `BM_CancelOrder/64`: 35.28 M ops/s
- `BM_CancelOrder/512`: 44.58 M ops/s
- `BM_MatchHeavyThroughput/64`: 25.43 M ops/s
- `BM_MatchHeavyThroughput/512`: 33.26 M ops/s

Raw benchmark output is in `benchmarks/baseline_benchmark.json`.
