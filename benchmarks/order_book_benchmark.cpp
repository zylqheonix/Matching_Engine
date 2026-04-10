#include "order_book.hpp"

#include <benchmark/benchmark.h>

static void BM_OrderBookDefaultConstruct(benchmark::State &state) {
  for (auto _ : state) {
    OrderBook book;
    benchmark::DoNotOptimize(book);
  }
}

BENCHMARK(BM_OrderBookDefaultConstruct);

BENCHMARK_MAIN();
