#include "order_book.hpp"

#include <chrono>

#include <gtest/gtest.h>

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

TEST(OrderBookLimit, BestAskEmptyReturnsZeroId) {
  OrderBook book;
  Order a = book.get_best_ask();
  EXPECT_EQ(a.id, 0u);
}

TEST(OrderBookLimit, BestBidEmptyReturnsZeroId) {
  OrderBook book;
  Order b = book.get_best_bid();
  EXPECT_EQ(b.id, 0u);
}

TEST(OrderBookLimit, SingleAskRestsAtBestAsk) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::SELL, 100, 5));
  Order a = book.get_best_ask();
  EXPECT_EQ(a.price, 100u);
  EXPECT_EQ(a.quantity, 5u);
}

TEST(OrderBookLimit, SingleBidRestsAtBestBid) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::BUY, 100, 5));
  Order b = book.get_best_bid();
  EXPECT_EQ(b.price, 100u);
  EXPECT_EQ(b.quantity, 5u);
}

TEST(OrderBookLimit, BuyMatchesAskQuantity) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::SELL, 100, 10));
  book.add_limit_order(make_order(2, Side::BUY, 100, 3));
  Order a = book.get_best_ask();
  EXPECT_EQ(a.price, 100u);
  EXPECT_EQ(a.quantity, 7u);
}

TEST(OrderBookLimit, SellMatchesBidQuantity) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::BUY, 100, 10));
  book.add_limit_order(make_order(2, Side::SELL, 100, 4));
  Order b = book.get_best_bid();
  EXPECT_EQ(b.price, 100u);
  EXPECT_EQ(b.quantity, 6u);
}

TEST(OrderBookLimit, BuyBelowAskDoesNotCross) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::SELL, 100, 10));
  book.add_limit_order(make_order(2, Side::BUY, 99, 2));
  EXPECT_EQ(book.get_best_ask().quantity, 10u);
  EXPECT_EQ(book.get_best_bid().price, 99u);
  EXPECT_EQ(book.get_best_bid().quantity, 2u);
}
