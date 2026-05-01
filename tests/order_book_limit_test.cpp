#include "order_book.hpp"

#include <chrono>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

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

Order make_market_order(uint64_t id, Side side, uint64_t quantity) {
  Order o{};
  o.id = id;
  o.type = OrderType::MARKET;
  o.side = side;
  o.quantity = quantity;
  o.timestamp = std::chrono::system_clock::now();
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
  ASSERT_TRUE(a.price.has_value());
  EXPECT_EQ(*a.price, 100u);
  EXPECT_EQ(a.quantity, 5u);
}

TEST(OrderBookLimit, SingleBidRestsAtBestBid) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::BUY, 100, 5));
  Order b = book.get_best_bid();
  ASSERT_TRUE(b.price.has_value());
  EXPECT_EQ(*b.price, 100u);
  EXPECT_EQ(b.quantity, 5u);
}

TEST(OrderBookLimit, BuyMatchesAskQuantity) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::SELL, 100, 10));
  book.add_limit_order(make_order(2, Side::BUY, 100, 3));
  Order a = book.get_best_ask();
  ASSERT_TRUE(a.price.has_value());
  EXPECT_EQ(*a.price, 100u);
  EXPECT_EQ(a.quantity, 7u);
}

TEST(OrderBookLimit, SellMatchesBidQuantity) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::BUY, 100, 10));
  book.add_limit_order(make_order(2, Side::SELL, 100, 4));
  Order b = book.get_best_bid();
  ASSERT_TRUE(b.price.has_value());
  EXPECT_EQ(*b.price, 100u);
  EXPECT_EQ(b.quantity, 6u);
}

TEST(OrderBookLimit, BuyBelowAskDoesNotCross) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::SELL, 100, 10));
  book.add_limit_order(make_order(2, Side::BUY, 99, 2));
  EXPECT_EQ(book.get_best_ask().quantity, 10u);
  ASSERT_TRUE(book.get_best_bid().price.has_value());
  EXPECT_EQ(*book.get_best_bid().price, 99u);
  EXPECT_EQ(book.get_best_bid().quantity, 2u);
}

TEST(OrderBookLimit, EmptyBookAddBuyRestsAndNoTrades) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::BUY, 100, 5));

  EXPECT_TRUE(trades.empty());
  Order bid = book.get_best_bid();
  EXPECT_EQ(bid.id, 1u);
  ASSERT_TRUE(bid.price.has_value());
  EXPECT_EQ(*bid.price, 100u);
  EXPECT_EQ(bid.quantity, 5u);
}

TEST(OrderBookLimit, TwoNonCrossingOrdersBothRestNoTrades) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::BUY, 99, 3));
  book.add_limit_order(make_order(2, Side::SELL, 100, 4));

  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(book.get_best_bid().id, 1u);
  EXPECT_EQ(book.get_best_ask().id, 2u);
}

TEST(OrderBookLimit, CrossingExactMatchOneTradeAndBothGone) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 5));
  book.add_limit_order(make_order(2, Side::BUY, 100, 5));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].price, 100u);
  EXPECT_EQ(trades[0].quantity, 5u);
  EXPECT_EQ(book.get_best_ask().id, 0u);
  EXPECT_EQ(book.get_best_bid().id, 0u);
}

TEST(OrderBookLimit, CrossingTakerLargerLeavesTakerRemainderResting) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 3));
  book.add_limit_order(make_order(2, Side::BUY, 100, 5));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 3u);
  Order best_bid = book.get_best_bid();
  EXPECT_EQ(best_bid.id, 2u);
  ASSERT_TRUE(best_bid.price.has_value());
  EXPECT_EQ(*best_bid.price, 100u);
  EXPECT_EQ(best_bid.quantity, 2u);
  EXPECT_EQ(book.get_best_ask().id, 0u);
}

TEST(OrderBookLimit, CrossingMakerLargerLeavesMakerRemainderResting) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 5));
  book.add_limit_order(make_order(2, Side::BUY, 100, 3));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 3u);
  Order best_ask = book.get_best_ask();
  EXPECT_EQ(best_ask.id, 1u);
  ASSERT_TRUE(best_ask.price.has_value());
  EXPECT_EQ(*best_ask.price, 100u);
  EXPECT_EQ(best_ask.quantity, 2u);
  EXPECT_EQ(book.get_best_bid().id, 0u);
}

TEST(OrderBookLimit, TakerSweepsTwoAtSamePriceInFifoOrder) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 2));
  book.add_limit_order(make_order(2, Side::SELL, 100, 2));
  book.add_limit_order(make_order(3, Side::BUY, 100, 3));

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].quantity, 2u);
  EXPECT_EQ(trades[1].quantity, 1u);
  EXPECT_EQ(trades[0].taker_order_id, 1u);
  EXPECT_EQ(trades[1].taker_order_id, 2u);
  EXPECT_EQ(book.get_best_ask().id, 2u);
  EXPECT_EQ(book.get_best_ask().quantity, 1u);
}

TEST(OrderBookLimit, CancelRestingOrderRemovesItAndPreventsMatching) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 3));
  EXPECT_TRUE(book.cancel_order(1));
  EXPECT_EQ(book.get_best_ask().id, 0u);

  book.add_limit_order(make_order(2, Side::BUY, 100, 3));
  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(book.get_best_bid().id, 2u);
  EXPECT_EQ(book.get_best_bid().quantity, 3u);
}

TEST(OrderBookLimit, CancelNonExistentOrderReturnsFalse) {
  OrderBook book;
  EXPECT_FALSE(book.cancel_order(42));
}

TEST(OrderBookLimit, BetterPriceMatchesFirstRegardlessOfArrivalTime) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 101, 1));
  book.add_limit_order(make_order(2, Side::SELL, 100, 1));
  book.add_limit_order(make_order(3, Side::BUY, 101, 1));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].price, 100u);
  EXPECT_EQ(book.get_best_ask().id, 1u);
  ASSERT_TRUE(book.get_best_ask().price.has_value());
  EXPECT_EQ(*book.get_best_ask().price, 101u);
}

TEST(OrderBookLimit, EarlierOrderAtSamePriceMatchesFirst) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 1));
  book.add_limit_order(make_order(2, Side::SELL, 100, 1));
  book.add_limit_order(make_order(3, Side::BUY, 100, 1));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].taker_order_id, 1u);
  EXPECT_EQ(book.get_best_ask().id, 2u);
  EXPECT_EQ(book.get_best_ask().quantity, 1u);
}

TEST(OrderBookLimit, MarketBuySweepsWithoutPriceConstraintAndDoesNotRest) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 101, 2));
  book.add_limit_order(make_order(2, Side::SELL, 102, 2));
  book.add_market_order(make_market_order(3, Side::BUY, 3));

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].price, 101u);
  EXPECT_EQ(trades[1].price, 102u);
  EXPECT_EQ(book.get_best_ask().id, 2u);
  EXPECT_EQ(book.get_best_ask().quantity, 1u);
  EXPECT_EQ(book.get_best_bid().id, 0u);
}

TEST(OrderBookLimit, MarketOrderCannotBeAddedAsLimit) {
  OrderBook book;
  EXPECT_THROW(book.add_limit_order(make_market_order(10, Side::BUY, 1)),
               std::invalid_argument);
}

TEST(OrderBookLimit, PartialFillAcrossMultiplePriceLevels) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 2));
  book.add_limit_order(make_order(2, Side::SELL, 101, 2));
  book.add_limit_order(make_order(3, Side::BUY, 101, 3));

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].price, 100u);
  EXPECT_EQ(trades[0].quantity, 2u);
  EXPECT_EQ(trades[1].price, 101u);
  EXPECT_EQ(trades[1].quantity, 1u);
  EXPECT_EQ(book.get_best_ask().id, 2u);
  EXPECT_EQ(book.get_best_ask().quantity, 1u);
}

TEST(OrderBookLimit, CancelThenReinsertSameIdCanMatchAgain) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(42, Side::SELL, 100, 2));
  ASSERT_TRUE(book.cancel_order(42));
  EXPECT_EQ(book.get_best_ask().id, 0u);

  book.add_limit_order(make_order(42, Side::SELL, 100, 2));
  book.add_limit_order(make_order(99, Side::BUY, 100, 2));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].taker_order_id, 42u);
  EXPECT_EQ(book.get_best_ask().id, 0u);
  EXPECT_EQ(book.get_best_bid().id, 0u);
}

TEST(OrderBookLimit, MarketOrderIntoEmptyBookProducesNoTradesAndNoRestingOrder) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_market_order(make_market_order(7, Side::BUY, 5));

  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(book.get_best_bid().id, 0u);
  EXPECT_EQ(book.get_best_ask().id, 0u);
}

TEST(OrderBookLimit, SelfCrossAttemptMatchesInCurrentModelWithoutOwnerIdentity) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  // Book has no account/owner identity, so "self-cross prevention" is out of scope.
  // This test locks in current behavior: crossing orders always match.
  book.add_limit_order(make_order(1, Side::SELL, 100, 1));
  book.add_limit_order(make_order(2, Side::BUY, 100, 1));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].maker_order_id, 2u);
  EXPECT_EQ(trades[0].taker_order_id, 1u);
}
