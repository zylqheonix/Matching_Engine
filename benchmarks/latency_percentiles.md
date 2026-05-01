# Order Book Latency Percentiles

- Samples per path: 1000000
- Unit: nanoseconds
- Build expectation: Release (`-O3 -DNDEBUG`)
- Machine: Apple M4 Max, 14 CPU cores
- Pinning note: this run was not core-pinned (macOS default scheduler)
- Workload shape:
  - add (no match): non-crossing BUYs against seeded asks; inserted order is canceled immediately
  - add (matches): BUYs crossing a steady ask queue at top of book
  - cancel: cancel against a fixed-size rotating ring of resting BUY orders

| Path | p50 | p90 | p99 | p99.9 | max |
| --- | ---: | ---: | ---: | ---: | ---: |
| add (no match) | 42 | 83 | 125 | 167 | 103250 |
| add (matches) | 42 | 42 | 83 | 333 | 1352917 |
| cancel | 41 | 42 | 42 | 84 | 6500 |
