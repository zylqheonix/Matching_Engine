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

TEST(OrderBookLimit, BestAskEmptyReturnsNullopt) {
  OrderBook book;
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookLimit, BestBidEmptyReturnsNullopt) {
  OrderBook book;
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookLimit, SingleAskRestsAtBestAsk) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::SELL, 100, 5));
  auto a = book.get_best_ask();
  ASSERT_TRUE(a);
  ASSERT_TRUE(a->price.has_value());
  EXPECT_EQ(*a->price, 100u);
  EXPECT_EQ(a->quantity, 5u);
}

TEST(OrderBookLimit, SingleBidRestsAtBestBid) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::BUY, 100, 5));
  auto b = book.get_best_bid();
  ASSERT_TRUE(b);
  ASSERT_TRUE(b->price.has_value());
  EXPECT_EQ(*b->price, 100u);
  EXPECT_EQ(b->quantity, 5u);
}

TEST(OrderBookLimit, BuyMatchesAskQuantity) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::SELL, 100, 10));
  book.add_limit_order(make_order(2, Side::BUY, 100, 3));
  auto a = book.get_best_ask();
  ASSERT_TRUE(a);
  ASSERT_TRUE(a->price.has_value());
  EXPECT_EQ(*a->price, 100u);
  EXPECT_EQ(a->quantity, 7u);
}

TEST(OrderBookLimit, SellMatchesBidQuantity) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::BUY, 100, 10));
  book.add_limit_order(make_order(2, Side::SELL, 100, 4));
  auto b = book.get_best_bid();
  ASSERT_TRUE(b);
  ASSERT_TRUE(b->price.has_value());
  EXPECT_EQ(*b->price, 100u);
  EXPECT_EQ(b->quantity, 6u);
}

TEST(OrderBookLimit, BuyBelowAskDoesNotCross) {
  OrderBook book;
  book.add_limit_order(make_order(1, Side::SELL, 100, 10));
  book.add_limit_order(make_order(2, Side::BUY, 99, 2));
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(ask->quantity, 10u);
  auto bid = book.get_best_bid();
  ASSERT_TRUE(bid);
  ASSERT_TRUE(bid->price.has_value());
  EXPECT_EQ(*bid->price, 99u);
  EXPECT_EQ(bid->quantity, 2u);
}

TEST(OrderBookLimit, EmptyBookAddBuyRestsAndNoTrades) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::BUY, 100, 5));

  EXPECT_TRUE(trades.empty());
  auto bid = book.get_best_bid();
  ASSERT_TRUE(bid);
  EXPECT_EQ(bid->id, 1u);
  ASSERT_TRUE(bid->price.has_value());
  EXPECT_EQ(*bid->price, 100u);
  EXPECT_EQ(bid->quantity, 5u);
}

TEST(OrderBookLimit, TwoNonCrossingOrdersBothRestNoTrades) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::BUY, 99, 3));
  book.add_limit_order(make_order(2, Side::SELL, 100, 4));

  EXPECT_TRUE(trades.empty());
  auto best_bid = book.get_best_bid();
  ASSERT_TRUE(best_bid);
  EXPECT_EQ(best_bid->id, 1u);
  auto best_ask = book.get_best_ask();
  ASSERT_TRUE(best_ask);
  EXPECT_EQ(best_ask->id, 2u);
}

TEST(OrderBookLimit, CrossingExactMatchOneTradeAndBothGone) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 5));
  book.add_limit_order(make_order(2, Side::BUY, 100, 5));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].price, 100u);
  EXPECT_EQ(trades[0].quantity, 5u);
  EXPECT_FALSE(book.get_best_ask().has_value());
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookLimit, CrossingTakerLargerLeavesTakerRemainderResting) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 3));
  book.add_limit_order(make_order(2, Side::BUY, 100, 5));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 3u);
  auto best_bid = book.get_best_bid();
  ASSERT_TRUE(best_bid);
  EXPECT_EQ(best_bid->id, 2u);
  ASSERT_TRUE(best_bid->price.has_value());
  EXPECT_EQ(*best_bid->price, 100u);
  EXPECT_EQ(best_bid->quantity, 2u);
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookLimit, CrossingMakerLargerLeavesMakerRemainderResting) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 5));
  book.add_limit_order(make_order(2, Side::BUY, 100, 3));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 3u);
  auto best_ask = book.get_best_ask();
  ASSERT_TRUE(best_ask);
  EXPECT_EQ(best_ask->id, 1u);
  ASSERT_TRUE(best_ask->price.has_value());
  EXPECT_EQ(*best_ask->price, 100u);
  EXPECT_EQ(best_ask->quantity, 2u);
  EXPECT_FALSE(book.get_best_bid().has_value());
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
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(ask->id, 2u);
  EXPECT_EQ(ask->quantity, 1u);
}

TEST(OrderBookLimit, CancelRestingOrderRemovesItAndPreventsMatching) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 3));
  EXPECT_TRUE(book.cancel_order(1));
  EXPECT_FALSE(book.get_best_ask().has_value());

  book.add_limit_order(make_order(2, Side::BUY, 100, 3));
  EXPECT_TRUE(trades.empty());
  auto bid = book.get_best_bid();
  ASSERT_TRUE(bid);
  EXPECT_EQ(bid->id, 2u);
  EXPECT_EQ(bid->quantity, 3u);
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
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(ask->id, 1u);
  ASSERT_TRUE(ask->price.has_value());
  EXPECT_EQ(*ask->price, 101u);
}

TEST(OrderBookLimit, EarlierOrderAtSamePriceMatchesFirst) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 1));
  book.add_limit_order(make_order(2, Side::SELL, 100, 1));
  book.add_limit_order(make_order(3, Side::BUY, 100, 1));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].taker_order_id, 1u);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(ask->id, 2u);
  EXPECT_EQ(ask->quantity, 1u);
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
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(ask->id, 2u);
  EXPECT_EQ(ask->quantity, 1u);
  EXPECT_FALSE(book.get_best_bid().has_value());
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
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(ask->id, 2u);
  EXPECT_EQ(ask->quantity, 1u);
}

TEST(OrderBookLimit, CancelThenReinsertSameIdCanMatchAgain) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &trade) { trades.push_back(trade); });

  book.add_limit_order(make_order(42, Side::SELL, 100, 2));
  ASSERT_TRUE(book.cancel_order(42));
  EXPECT_FALSE(book.get_best_ask().has_value());

  book.add_limit_order(make_order(42, Side::SELL, 100, 2));
  book.add_limit_order(make_order(99, Side::BUY, 100, 2));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].taker_order_id, 42u);
  EXPECT_FALSE(book.get_best_ask().has_value());
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookLimit, MarketOrderIntoEmptyBookProducesNoTradesAndNoRestingOrder) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book(
      [&trades](const Trade &trade) { trades.push_back(trade); },
      [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_market_order(make_market_order(7, Side::BUY, 5));

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, 7u);
  EXPECT_EQ(ioc[0].side, Side::BUY);
  EXPECT_EQ(ioc[0].canceled_quantity, 5u);
  EXPECT_FALSE(book.get_best_bid().has_value());
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookLimit, MarketOrderIocEmitsCanceledRemainderAfterPartialFill) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book(
      [&trades](const Trade &trade) { trades.push_back(trade); },
      [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 2));
  book.add_market_order(make_market_order(2, Side::BUY, 10));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 2u);
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, 2u);
  EXPECT_EQ(ioc[0].side, Side::BUY);
  EXPECT_EQ(ioc[0].canceled_quantity, 8u);
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookIoc, MarketSellIntoEmptyBookEmitsIocCanceled) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book(
      [&trades](const Trade &trade) { trades.push_back(trade); },
      [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_market_order(make_market_order(9, Side::SELL, 4));

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, 9u);
  EXPECT_EQ(ioc[0].side, Side::SELL);
  EXPECT_EQ(ioc[0].canceled_quantity, 4u);
}

TEST(OrderBookIoc, MarketSellPartialFillEmitsIocForRemainder) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book(
      [&trades](const Trade &trade) { trades.push_back(trade); },
      [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(make_order(1, Side::BUY, 100, 3));
  book.add_market_order(make_market_order(2, Side::SELL, 10));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 3u);
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, 2u);
  EXPECT_EQ(ioc[0].side, Side::SELL);
  EXPECT_EQ(ioc[0].canceled_quantity, 7u);
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookIoc, FullyFilledMarketOrderDoesNotEmitIocCanceled) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book(
      [&trades](const Trade &trade) { trades.push_back(trade); },
      [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(make_order(1, Side::SELL, 100, 5));
  book.add_market_order(make_market_order(2, Side::BUY, 5));

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 5u);
  EXPECT_TRUE(ioc.empty());
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookIoc, SetIocCallbackAfterConstructionIsUsed) {
  std::vector<IocCanceled> ioc;
  OrderBook book;
  book.set_ioc_canceled_callback(
      [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_market_order(make_market_order(100, Side::BUY, 2));

  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, 100u);
  EXPECT_EQ(ioc[0].canceled_quantity, 2u);
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
