#include "order_book.hpp"

#include <chrono>

#include <benchmark/benchmark.h>

namespace {

Order make_order(uint64_t id, Side side, uint64_t price, uint64_t quantity) {
  Order o{};
  o.id = id;
  o.side = side;
  o.price = price;
  o.quantity = quantity;
  o.timestamp = std::chrono::system_clock::now();
  o.is_filled = false;
  return o;
}

} // namespace

static void BM_AddLimitOrder(benchmark::State &state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    OrderBook book;
    for (int i = 0; i < n; ++i) {
      const uint64_t id = static_cast<uint64_t>(i + 1);
      const Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
      const uint64_t price = 100 + static_cast<uint64_t>(i % 20);
      book.add_limit_order(make_order(id, side, price, 1));
    }
    benchmark::DoNotOptimize(book.get_best_bid());
    benchmark::DoNotOptimize(book.get_best_ask());
  }
  state.SetItemsProcessed(state.iterations() * n);
}

BENCHMARK(BM_AddLimitOrder)->Arg(64)->Arg(512);

BENCHMARK_MAIN();
