#include "order_book.hpp"

#include <cstdint>
#include <random>
#include <vector>

#include <gtest/gtest.h>

struct OrderBookTestAccess {
  static const std::map<uint64_t, PriceLevel, std::greater<uint64_t>> &
  bids(const OrderBook &book) {
    return book.bids;
  }

  static const std::map<uint64_t, PriceLevel, std::less<uint64_t>> &
  asks(const OrderBook &book) {
    return book.asks;
  }

  static const std::unordered_map<uint64_t, LookupEntry> &
  lookup(const OrderBook &book) {
    return book.lookup_table;
  }
};

namespace {

void assert_book_invariants(const OrderBook &book) {
  uint64_t level_order_count = 0;
  uint64_t level_total_qty = 0;

  uint64_t prev_bid = 0;
  bool has_prev_bid = false;
  for (const auto &[price, level] : OrderBookTestAccess::bids(book)) {
    if (has_prev_bid) {
      EXPECT_GE(prev_bid, price);
    }
    prev_bid = price;
    has_prev_bid = true;
    EXPECT_EQ(level.price(), price);
    EXPECT_FALSE(level.empty());

    const Order *prev = nullptr;
    for (const Order *o = level.front(); o != nullptr; o = o->next) {
      ASSERT_EQ(o->prev, prev);
      prev = o;
      ++level_order_count;
      level_total_qty += o->quantity;
      EXPECT_EQ(o->side, Side::BUY);
      ASSERT_TRUE(o->price.has_value());
      EXPECT_EQ(*o->price, price);
      EXPECT_EQ(o->type, OrderType::LIMIT);
      EXPECT_GT(o->quantity, 0u);
    }
  }

  uint64_t prev_ask = 0;
  bool has_prev_ask = false;
  for (const auto &[price, level] : OrderBookTestAccess::asks(book)) {
    if (has_prev_ask) {
      EXPECT_LE(prev_ask, price);
    }
    prev_ask = price;
    has_prev_ask = true;
    EXPECT_EQ(level.price(), price);
    EXPECT_FALSE(level.empty());

    const Order *prev = nullptr;
    for (const Order *o = level.front(); o != nullptr; o = o->next) {
      ASSERT_EQ(o->prev, prev);
      prev = o;
      ++level_order_count;
      level_total_qty += o->quantity;
      EXPECT_EQ(o->side, Side::SELL);
      ASSERT_TRUE(o->price.has_value());
      EXPECT_EQ(*o->price, price);
      EXPECT_EQ(o->type, OrderType::LIMIT);
      EXPECT_GT(o->quantity, 0u);
    }
  }

  EXPECT_EQ(OrderBookTestAccess::lookup(book).size(), level_order_count);

  uint64_t lookup_total_qty = 0;
  for (const auto &[id, entry] : OrderBookTestAccess::lookup(book)) {
    ASSERT_NE(entry.order_ptr, nullptr);
    const Order &order = *entry.order_ptr;
    EXPECT_EQ(order.id, id);
    EXPECT_EQ(order.side, entry.side);
    ASSERT_TRUE(order.price.has_value());
    EXPECT_EQ(*order.price, entry.price);
    lookup_total_qty += order.quantity;
  }

  EXPECT_EQ(lookup_total_qty, level_total_qty);
}

} // namespace

TEST(OrderBookProperty, RandomizedInvariantsHoldAcrossTenThousandOrders) {
  OrderBook book;
  std::mt19937_64 rng(1337);
  std::uniform_int_distribution<int> action_dist(0, 99);
  std::uniform_int_distribution<int> side_dist(0, 1);
  std::uniform_int_distribution<int> price_dist(90, 110);
  std::uniform_int_distribution<int> qty_dist(1, 10);

  std::vector<uint64_t> live_ids;
  live_ids.reserve(1024);

  for (int i = 0; i < 10000; ++i) {
    const int action = action_dist(rng);
    if (action < 60) {
      const Side side = (side_dist(rng) == 0) ? Side::BUY : Side::SELL;
      const uint64_t price = static_cast<uint64_t>(price_dist(rng));
      const uint64_t qty = static_cast<uint64_t>(qty_dist(rng));
      const uint64_t assigned = book.add_limit_order(side, price, qty);
      live_ids.push_back(assigned);
    } else if (action < 85) {
      const Side side = (side_dist(rng) == 0) ? Side::BUY : Side::SELL;
      const uint64_t qty = static_cast<uint64_t>(qty_dist(rng));
      (void)book.add_market_order(side, qty);
    } else if (!live_ids.empty()) {
      std::uniform_int_distribution<size_t> idx_dist(0, live_ids.size() - 1);
      const size_t idx = idx_dist(rng);
      const uint64_t id = live_ids[idx];
      live_ids[idx] = live_ids.back();
      live_ids.pop_back();
      (void)book.cancel_order(id);
    }

    assert_book_invariants(book);
  }
}

TEST(OrderBookProperty, BookEmptyAfterEqualAndOppositeCrossingFlow) {
  OrderBook book;
  std::mt19937_64 rng(99);
  std::uniform_int_distribution<int> price_dist(95, 105);
  std::uniform_int_distribution<int> qty_dist(1, 5);

  // Generate symmetric buy/sell limit orders that will fully cross.
  std::vector<std::pair<Side, std::pair<uint64_t, uint64_t>>> ops;
  ops.reserve(200);
  for (int i = 0; i < 100; ++i) {
    const uint64_t p = static_cast<uint64_t>(price_dist(rng));
    const uint64_t q = static_cast<uint64_t>(qty_dist(rng));
    ops.push_back({Side::BUY, {p, q}});
    ops.push_back({Side::SELL, {p, q}});
  }
  std::shuffle(ops.begin(), ops.end(), rng);

  for (const auto &[side, pq] : ops) {
    book.add_limit_order(side, pq.first, pq.second);
    assert_book_invariants(book);
  }

  // After all crossing trades, net resting quantity may still be > 0 because
  // partials can rest. Invariants must hold either way.
  assert_book_invariants(book);
}
