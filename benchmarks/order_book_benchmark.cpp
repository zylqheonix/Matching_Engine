#include "order_book.hpp"

#include <vector>

#include <benchmark/benchmark.h>

static void BM_AddLimitOrder(benchmark::State &state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    OrderBook book;
    for (int i = 0; i < n; ++i) {
      const Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
      const uint64_t price = 100 + static_cast<uint64_t>(i % 20);
      book.add_limit_order(side, price, 1);
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
      ids.push_back(book.add_limit_order(Side::BUY, 100, 1));
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
      const uint64_t price = 100 + static_cast<uint64_t>(i % 10);
      book.add_limit_order(Side::SELL, price, 1);
    }
    state.ResumeTiming();

    for (int i = 0; i < n; ++i) {
      book.add_limit_order(Side::BUY, 110, 1);
    }
  }
  state.SetItemsProcessed(state.iterations() * n);
}

BENCHMARK(BM_MatchHeavyThroughput)->Arg(64)->Arg(512);

static void BM_AddLimitOrderIocNoMatch(benchmark::State &state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    state.PauseTiming();
    OrderBook book;
    for (int i = 0; i < n; ++i) {
      book.add_limit_order(Side::SELL, 200, 1);
    }
    state.ResumeTiming();

    for (int i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(book.add_limit_order_IOC(Side::BUY, 100, 1));
    }
  }
  state.SetItemsProcessed(state.iterations() * n);
}

BENCHMARK(BM_AddLimitOrderIocNoMatch)->Arg(64)->Arg(512);

static void BM_AddLimitOrderIocMatch(benchmark::State &state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    state.PauseTiming();
    OrderBook book;
    for (int i = 0; i < n; ++i) {
      const uint64_t price = 100 + static_cast<uint64_t>(i % 10);
      book.add_limit_order(Side::SELL, price, 1);
    }
    state.ResumeTiming();

    for (int i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(book.add_limit_order_IOC(Side::BUY, 110, 1));
    }
  }
  state.SetItemsProcessed(state.iterations() * n);
}

BENCHMARK(BM_AddLimitOrderIocMatch)->Arg(64)->Arg(512);

static void BM_AddLimitOrderIocPartialFill(benchmark::State &state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    state.PauseTiming();
    OrderBook book;
    for (int i = 0; i < n; ++i) {
      book.add_limit_order(Side::SELL, 100, 1);
    }
    state.ResumeTiming();

    for (int i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(book.add_limit_order_IOC(Side::BUY, 100, 2));
    }
  }
  state.SetItemsProcessed(state.iterations() * n);
}

BENCHMARK(BM_AddLimitOrderIocPartialFill)->Arg(64)->Arg(512);

BENCHMARK_MAIN();
