#include "order.hpp"
#include "order_book.hpp"

#include <chrono>

#include <gtest/gtest.h>

TEST(OrderInit, BracedInitializer) {
  Order o{};
  o.id = 1;
  o.side = Side::BUY;
  o.price = 100.0;
  o.quantity = 1.0;
  o.timestamp = std::chrono::system_clock::now();
  o.is_filled = false;
  EXPECT_EQ(o.id, 1u);
  EXPECT_EQ(o.side, Side::BUY);
  EXPECT_DOUBLE_EQ(o.price, 100.0);
}

TEST(OrderBookInit, DefaultConstructs) {
  OrderBook book;
  (void)book;
}

TEST(OrderBookInit, ManyDefaultConstructs) {
  constexpr int kN = 1000;
  for (int i = 0; i < kN; ++i) {
    OrderBook book;
    (void)book;
  }
}
