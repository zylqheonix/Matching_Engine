#include "order_book.hpp"

#include <chrono>
#include <vector>

#include <benchmark/benchmark.h>

namespace {

Order make_order(uint64_t id, Side side, uint64_t price, uint64_t quantity) {
  Order o{};
  o.id = id;
  o.type = OrderType::LIMIT;
  o.side = side;
  o.price = price;
  o.quantity = quantity;
  o.timestamp = std::chrono::system_clock::now();
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

static void BM_CancelOrder(benchmark::State &state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    state.PauseTiming();
    OrderBook book;
    std::vector<uint64_t> ids;
    ids.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
      const uint64_t id = static_cast<uint64_t>(i + 1);
      ids.push_back(id);
      book.add_limit_order(make_order(id, Side::BUY, 100, 1));
    }
    state.ResumeTiming();

    for (uint64_t id : ids) {
      benchmark::DoNotOptimize(book.cancel_order(id));
    }
  }
  state.SetItemsProcessed(state.iterations() * n);
}

BENCHMARK(BM_CancelOrder)->Arg(64)->Arg(512);

static void BM_MatchHeavyThroughput(benchmark::State &state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    state.PauseTiming();
    OrderBook book;
    for (int i = 0; i < n; ++i) {
      const uint64_t id = static_cast<uint64_t>(i + 1);
      const uint64_t price = 100 + static_cast<uint64_t>(i % 10);
      book.add_limit_order(make_order(id, Side::SELL, price, 1));
    }
    state.ResumeTiming();

    for (int i = 0; i < n; ++i) {
      const uint64_t id = static_cast<uint64_t>(n + i + 1);
      book.add_limit_order(make_order(id, Side::BUY, 110, 1));
    }
  }
  state.SetItemsProcessed(state.iterations() * n);
}

BENCHMARK(BM_MatchHeavyThroughput)->Arg(64)->Arg(512);

BENCHMARK_MAIN();
