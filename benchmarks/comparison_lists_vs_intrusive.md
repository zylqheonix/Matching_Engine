# Benchmark Comparison: `std::list` vs Intrusive List + OrderPool

Captured 2026-05-26 on Apple Silicon (Release, `-O3 -DNDEBUG`).

## What changed

| Component | Before (`5c731d1`) | After (intrusive) |
| --- | --- | --- |
| Per-level queue | `std::list<Order>` | `PriceLevel` (intrusive `Order::next` / `prev`) |
| Resting order storage | Copied into list nodes | `OrderPool` slab allocation (`Order*`) |
| Lookup | `std::list<Order>::iterator` | `Order*` |

Baseline list numbers were captured by checking out `5c731d1` and running the same benchmark binaries. Intrusive numbers are from the current tree.

## Throughput (`order_book_benchmark`)

| Benchmark | List (time) | List (items/s) | Intrusive (time) | Intrusive (items/s) | Δ throughput |
| --- | ---: | ---: | ---: | ---: | ---: |
| AddLimitOrder/64 | 2788 ns | 23.0 M/s | 2929 ns | 21.9 M/s | **-4.8%** |
| AddLimitOrder/512 | 22005 ns | 23.3 M/s | 19556 ns | 26.3 M/s | **+12.9%** |
| CancelOrder/64 | 1740 ns | 36.8 M/s | 1415 ns | 45.3 M/s | **+23.1%** |
| CancelOrder/512 | 11009 ns | 46.6 M/s | 8068 ns | 63.8 M/s | **+36.9%** |
| MatchHeavy/64 | 2463 ns | 26.0 M/s | 2139 ns | 30.0 M/s | **+15.4%** |
| MatchHeavy/512 | 15218 ns | 33.7 M/s | 12135 ns | 42.3 M/s | **+25.5%** |

### Throughput takeaways

- **Cancel is the biggest win** — removing a node from an intrusive list + returning to a free list beats `std::list::erase` + destructor path. Gains grow with book depth (512 >> 64).
- **Match-heavy paths improve** — full fills deallocate from the pool instead of destroying list nodes; less allocator traffic under churn.
- **Small add-only batches (64) are slightly slower** — pool allocate + pointer wiring has a fixed cost that can dominate when the book is tiny and nothing matches. At 512 resting orders the pool amortizes and add becomes faster than lists.
- Net: the refactor pays off on realistic hot paths (cancel, match, deeper books) even if micro add-only at n=64 regresses slightly.

## Latency percentiles (`order_book_latency_benchmark`, 200k samples)

| Path | Metric | List (ns) | Intrusive (ns) |
| --- | --- | ---: | ---: |
| add (no match) | p50 | 83 | 83 |
| add (no match) | p99 | 125 | 125 |
| add (matches) | p50 | 42 | 42 |
| add (matches) | p99 | 84 | 84 |
| cancel | p50 | 41 | 41 |
| cancel | p99 | 42 | 42 |

Tail max values differ run-to-run (OS jitter, thermal state) and are not directly comparable across sessions. Median and p99 paths are effectively unchanged — the throughput gains show up as higher sustained ops/s rather than a dramatically different single-op p50.

## Files

- `benchmarks/baseline.md` — older list baseline snapshot
- `benchmarks/intrusive_latency.md` — current latency percentiles
- `benchmarks/comparison_lists_vs_intrusive.md` — this document
