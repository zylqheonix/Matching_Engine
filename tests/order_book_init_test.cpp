#include "order.hpp"
#include "order_book.hpp"

#include <chrono>

#include <gtest/gtest.h>

TEST(OrderInit, BracedInitializer) {
  Order o{1,
          Side::BUY,
          100.0,
          1.0,
          std::chrono::system_clock::now(),
          false};
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
