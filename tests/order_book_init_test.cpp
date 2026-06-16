#include "order_book.hpp"

#include <gtest/gtest.h>

TEST(OrderBookInit, DefaultConstructs) {
  OrderBook book;
  EXPECT_FALSE(book.get_best_ask().has_value());
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookInit, ManyDefaultConstructs) {
  constexpr int kN = 1000;
  for (int i = 0; i < kN; ++i) {
    OrderBook book;
    (void)book;
  }
}

TEST(OrderBookInit, AssignsOrderIdsStartingAtOne) {
  OrderBook book;
  EXPECT_EQ(book.add_limit_order(Side::BUY, 100, 1), 1u);
  EXPECT_EQ(book.add_limit_order(Side::SELL, 200, 1), 2u);
  EXPECT_EQ(book.add_market_order(Side::BUY, 1), 3u);
}
