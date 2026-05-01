#include "order.hpp"
#include "order_book.hpp"

#include <chrono>

#include <gtest/gtest.h>

TEST(OrderInit, BracedInitializer) {
  Order o{};
  o.id = 1;
  o.type = OrderType::LIMIT;
  o.side = Side::BUY;
  o.price = 100;
  o.quantity = 1;
  o.timestamp = std::chrono::system_clock::now();
  EXPECT_EQ(o.id, 1u);
  EXPECT_EQ(o.type, OrderType::LIMIT);
  EXPECT_EQ(o.side, Side::BUY);
  ASSERT_TRUE(o.price.has_value());
  EXPECT_EQ(*o.price, 100u);
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
