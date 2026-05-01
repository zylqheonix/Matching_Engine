#include "order_book.hpp"

#include <chrono>
#include <cstdint>
#include <random>

#include <gtest/gtest.h>

struct OrderBookTestAccess {
  static const std::map<uint64_t, std::list<Order>, std::greater<uint64_t>> &
  bids(const OrderBook &book) {
    return book.bids;
  }

  static const std::map<uint64_t, std::list<Order>, std::less<uint64_t>> &
  asks(const OrderBook &book) {
    return book.asks;
  }

  static const std::unordered_map<uint64_t, LookupEntry> &
  lookup(const OrderBook &book) {
    return book.lookup_table;
  }
};

namespace {

Order make_limit_order(uint64_t id, Side side, uint64_t price, uint64_t quantity) {
  Order o{};
  o.id = id;
  o.type = OrderType::LIMIT;
  o.side = side;
  o.quantity = quantity;
  o.price = price;
  o.timestamp = std::chrono::system_clock::now();
  return o;
}

Order make_market_order(uint64_t id, Side side, uint64_t quantity) {
  Order o{};
  o.id = id;
  o.type = OrderType::MARKET;
  o.side = side;
  o.quantity = quantity;
  o.timestamp = std::chrono::system_clock::now();
  return o;
}

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
    level_order_count += static_cast<uint64_t>(level.size());
    for (const auto &order : level) {
      level_total_qty += order.quantity;
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
    level_order_count += static_cast<uint64_t>(level.size());
    for (const auto &order : level) {
      level_total_qty += order.quantity;
    }
  }

  EXPECT_EQ(OrderBookTestAccess::lookup(book).size(), level_order_count);

  uint64_t lookup_total_qty = 0;
  for (const auto &[id, entry] : OrderBookTestAccess::lookup(book)) {
    const Order &order = *entry.order_iterator;
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
  std::uniform_int_distribution<int> type_dist(0, 99);
  std::uniform_int_distribution<int> side_dist(0, 1);
  std::uniform_int_distribution<int> price_dist(90, 110);
  std::uniform_int_distribution<int> qty_dist(1, 10);
  std::uniform_int_distribution<int> cancel_dist(1, 12000);

  uint64_t next_id = 1;
  for (int i = 0; i < 10000; ++i) {
    const int action = type_dist(rng);
    if (action < 60) {
      const Side side = (side_dist(rng) == 0) ? Side::BUY : Side::SELL;
      const uint64_t price = static_cast<uint64_t>(price_dist(rng));
      const uint64_t qty = static_cast<uint64_t>(qty_dist(rng));
      book.add_limit_order(make_limit_order(next_id++, side, price, qty));
    } else if (action < 85) {
      const Side side = (side_dist(rng) == 0) ? Side::BUY : Side::SELL;
      const uint64_t qty = static_cast<uint64_t>(qty_dist(rng));
      book.add_market_order(make_market_order(next_id++, side, qty));
    } else {
      const uint64_t id =
          static_cast<uint64_t>(cancel_dist(rng));
      (void)book.cancel_order(id);
    }

    assert_book_invariants(book);
  }
}
