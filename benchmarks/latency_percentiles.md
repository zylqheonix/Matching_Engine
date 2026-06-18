# Order Book Latency Percentiles

- Samples per path: 200000
- Unit: nanoseconds
- Build expectation: Release (`-O3 -DNDEBUG`)

| Path | p50 | p90 | p99 | p99.9 | max |
| --- | ---: | ---: | ---: | ---: | ---: |
| add (no match) | 83 | 84 | 125 | 208 | 19292 |
| add (matches) | 42 | 42 | 84 | 500 | 37166 |
| cancel | 41 | 42 | 42 | 83 | 20916 |
| limit IOC (no match) | 41 | 42 | 42 | 84 | 32292 |
| limit IOC (matches) | 42 | 42 | 42 | 125 | 15708 |
| limit IOC (partial fill) | 42 | 42 | 83 | 125 | 18625 |
| limit FOK (no match) | 41 | 42 | 42 | 84 | 13041 |
| limit FOK (kill) | 42 | 42 | 42 | 125 | 12208 |
| limit FOK (matches) | 42 | 42 | 83 | 125 | 43625 |
