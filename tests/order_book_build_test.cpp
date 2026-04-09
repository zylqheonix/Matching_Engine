#include "order_book.hpp"

#include <chrono>

#include <gtest/gtest.h>

TEST(OrderBookBuildTest, AddOrderInLoop) {
  OrderBook book;

  constexpr int kIterations = 1000;
  for (int i = 0; i < kIterations; ++i) {
    Order order{static_cast<uint64_t>(i + 1),
                (i % 2 == 0) ? Side::BUY : Side::SELL,
                100.0 + static_cast<double>(i),
                1.0,
                std::chrono::system_clock::now(),
                false};

    book.add_order(order);
    Order latest = book.get_latest_order();
    EXPECT_EQ(latest.id, order.id);
    EXPECT_EQ(latest.side, order.side);
  }
}
