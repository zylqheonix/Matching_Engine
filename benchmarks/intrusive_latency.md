# Order Book Latency Percentiles

- Samples per path: 200000
- Unit: nanoseconds
- Build expectation: Release (`-O3 -DNDEBUG`)

| Path | p50 | p90 | p99 | p99.9 | max |
| --- | ---: | ---: | ---: | ---: | ---: |
| add (no match) | 83 | 84 | 125 | 208 | 30875 |
| add (matches) | 42 | 42 | 84 | 166 | 12917 |
| cancel | 41 | 42 | 42 | 84 | 18417 |
