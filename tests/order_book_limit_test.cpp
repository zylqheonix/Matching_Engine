#include "order_book.hpp"

#include <vector>

#include <gtest/gtest.h>

// ---- get_best_* on empty book ----

TEST(OrderBookLimit, BestAskEmptyReturnsNullopt) {
  OrderBook book;
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookLimit, BestBidEmptyReturnsNullopt) {
  OrderBook book;
  EXPECT_FALSE(book.get_best_bid().has_value());
}

// ---- single-sided resting ----

TEST(OrderBookLimit, SingleAskRestsAtBestAsk) {
  OrderBook book;
  book.add_limit_order(Side::SELL, 100, 5);
  auto a = book.get_best_ask();
  ASSERT_TRUE(a);
  EXPECT_EQ(*a, 100u);
}

TEST(OrderBookLimit, SingleBidRestsAtBestBid) {
  OrderBook book;
  book.add_limit_order(Side::BUY, 100, 5);
  auto b = book.get_best_bid();
  ASSERT_TRUE(b);
  EXPECT_EQ(*b, 100u);
}

// ---- crossing & matching ----

TEST(OrderBookLimit, BuyMatchesAskQuantity) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::SELL, 100, 10);
  book.add_limit_order(Side::BUY, 100, 3);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].price, 100u);
  EXPECT_EQ(trades[0].quantity, 3u);
  auto a = book.get_best_ask();
  ASSERT_TRUE(a);
  EXPECT_EQ(*a, 100u);
}

TEST(OrderBookLimit, SellMatchesBidQuantity) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::BUY, 100, 10);
  book.add_limit_order(Side::SELL, 100, 4);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 4u);
  auto b = book.get_best_bid();
  ASSERT_TRUE(b);
  EXPECT_EQ(*b, 100u);
}

TEST(OrderBookLimit, BuyBelowAskDoesNotCross) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::SELL, 100, 10);
  book.add_limit_order(Side::BUY, 99, 2);

  EXPECT_TRUE(trades.empty());
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 100u);
  auto bid = book.get_best_bid();
  ASSERT_TRUE(bid);
  EXPECT_EQ(*bid, 99u);
}

TEST(OrderBookLimit, EmptyBookAddBuyRestsAndNoTrades) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::BUY, 100, 5);

  EXPECT_TRUE(trades.empty());
  auto bid = book.get_best_bid();
  ASSERT_TRUE(bid);
  EXPECT_EQ(*bid, 100u);
}

TEST(OrderBookLimit, TwoNonCrossingOrdersBothRestNoTrades) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::BUY, 99, 3);
  book.add_limit_order(Side::SELL, 100, 4);

  EXPECT_TRUE(trades.empty());
  auto bid = book.get_best_bid();
  ASSERT_TRUE(bid);
  EXPECT_EQ(*bid, 99u);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 100u);
}

TEST(OrderBookLimit, CrossingExactMatchOneTradeAndBothGone) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::SELL, 100, 5);
  book.add_limit_order(Side::BUY, 100, 5);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].price, 100u);
  EXPECT_EQ(trades[0].quantity, 5u);
  EXPECT_FALSE(book.get_best_ask().has_value());
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookLimit, CrossingTakerLargerLeavesTakerRemainderResting) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::SELL, 100, 3);
  book.add_limit_order(Side::BUY, 100, 5);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 3u);
  auto bid = book.get_best_bid();
  ASSERT_TRUE(bid);
  EXPECT_EQ(*bid, 100u);
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookLimit, CrossingMakerLargerLeavesMakerRemainderResting) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::SELL, 100, 5);
  book.add_limit_order(Side::BUY, 100, 3);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 3u);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 100u);
  EXPECT_FALSE(book.get_best_bid().has_value());
}

// ---- FIFO / price-time priority ----

TEST(OrderBookLimit, TakerSweepsTwoAtSamePriceInFifoOrder) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  const uint64_t s1 = book.add_limit_order(Side::SELL, 100, 2);
  const uint64_t s2 = book.add_limit_order(Side::SELL, 100, 2);
  book.add_limit_order(Side::BUY, 100, 3);

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].quantity, 2u);
  EXPECT_EQ(trades[1].quantity, 1u);
  // Trade struct has the maker id stored under taker_order_id (preserved from
  // the original list-based implementation; left intact during the pool/level
  // migration to keep semantics identical).
  EXPECT_EQ(trades[0].taker_order_id, s1);
  EXPECT_EQ(trades[1].taker_order_id, s2);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 100u);
}

TEST(OrderBookLimit, BetterPriceMatchesFirstRegardlessOfArrivalTime) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::SELL, 101, 1);
  book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order(Side::BUY, 101, 1);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].price, 100u);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 101u);
}

TEST(OrderBookLimit, EarlierOrderAtSamePriceMatchesFirst) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  const uint64_t s1 = book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order(Side::BUY, 100, 1);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].taker_order_id, s1);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 100u);
}

TEST(OrderBookLimit, PartialFillAcrossMultiplePriceLevels) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::SELL, 100, 2);
  book.add_limit_order(Side::SELL, 101, 2);
  book.add_limit_order(Side::BUY, 101, 3);

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].price, 100u);
  EXPECT_EQ(trades[0].quantity, 2u);
  EXPECT_EQ(trades[1].price, 101u);
  EXPECT_EQ(trades[1].quantity, 1u);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 101u);
}

// ---- cancel ----

TEST(OrderBookLimit, CancelRestingOrderRemovesItAndPreventsMatching) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  const uint64_t s1 = book.add_limit_order(Side::SELL, 100, 3);
  EXPECT_TRUE(book.cancel_order(s1));
  EXPECT_FALSE(book.get_best_ask().has_value());

  book.add_limit_order(Side::BUY, 100, 3);
  EXPECT_TRUE(trades.empty());
  auto bid = book.get_best_bid();
  ASSERT_TRUE(bid);
  EXPECT_EQ(*bid, 100u);
}

TEST(OrderBookLimit, CancelNonExistentOrderReturnsFalse) {
  OrderBook book;
  EXPECT_FALSE(book.cancel_order(42));
}

TEST(OrderBookLimit, DoubleCancelReturnsFalseSecondTime) {
  OrderBook book;
  const uint64_t s1 = book.add_limit_order(Side::SELL, 100, 1);
  EXPECT_TRUE(book.cancel_order(s1));
  EXPECT_FALSE(book.cancel_order(s1));
}

TEST(OrderBookLimit, CancelOneOrderAtLevelLeavesOthersIntact) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  const uint64_t s1 = book.add_limit_order(Side::SELL, 100, 1);
  const uint64_t s2 = book.add_limit_order(Side::SELL, 100, 1);
  const uint64_t s3 = book.add_limit_order(Side::SELL, 100, 1);

  EXPECT_TRUE(book.cancel_order(s2));
  book.add_limit_order(Side::BUY, 100, 3);

  // Only s1 and s3 should have matched (s2 canceled).
  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].taker_order_id, s1);
  EXPECT_EQ(trades[1].taker_order_id, s3);
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookLimit, CancelLastOrderRemovesEmptyLevel) {
  OrderBook book;
  const uint64_t s = book.add_limit_order(Side::SELL, 100, 5);
  EXPECT_TRUE(book.cancel_order(s));
  EXPECT_FALSE(book.get_best_ask().has_value());

  // Re-adding at the same price should rest cleanly.
  book.add_limit_order(Side::SELL, 100, 1);
  auto a = book.get_best_ask();
  ASSERT_TRUE(a);
  EXPECT_EQ(*a, 100u);
}

// ---- market / IOC ----

TEST(OrderBookLimit, MarketBuySweepsWithoutPriceConstraintAndDoesNotRest) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::SELL, 101, 2);
  book.add_limit_order(Side::SELL, 102, 2);
  book.add_market_order(Side::BUY, 3);

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].price, 101u);
  EXPECT_EQ(trades[1].price, 102u);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 102u);
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookLimit,
     MarketOrderIntoEmptyBookProducesNoTradesAndNoRestingOrder) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  const uint64_t m = book.add_market_order(Side::BUY, 5);

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, m);
  EXPECT_EQ(ioc[0].side, Side::BUY);
  EXPECT_EQ(ioc[0].canceled_quantity, 5u);
  EXPECT_FALSE(book.get_best_bid().has_value());
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookLimit, MarketOrderIocEmitsCanceledRemainderAfterPartialFill) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::SELL, 100, 2);
  const uint64_t m = book.add_market_order(Side::BUY, 10);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 2u);
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, m);
  EXPECT_EQ(ioc[0].side, Side::BUY);
  EXPECT_EQ(ioc[0].canceled_quantity, 8u);
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookIoc, MarketSellIntoEmptyBookEmitsIocCanceled) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  const uint64_t m = book.add_market_order(Side::SELL, 4);

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, m);
  EXPECT_EQ(ioc[0].side, Side::SELL);
  EXPECT_EQ(ioc[0].canceled_quantity, 4u);
}

TEST(OrderBookIoc, MarketSellPartialFillEmitsIocForRemainder) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::BUY, 100, 3);
  const uint64_t m = book.add_market_order(Side::SELL, 10);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 3u);
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, m);
  EXPECT_EQ(ioc[0].side, Side::SELL);
  EXPECT_EQ(ioc[0].canceled_quantity, 7u);
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookIoc, FullyFilledMarketOrderDoesNotEmitIocCanceled) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::SELL, 100, 5);
  book.add_market_order(Side::BUY, 5);

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

  const uint64_t m = book.add_market_order(Side::BUY, 2);

  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, m);
  EXPECT_EQ(ioc[0].canceled_quantity, 2u);
}

// ---- limit IOC ----

TEST(OrderBookLimitIoc, NonCrossingBuyIntoEmptyBookEmitsFullIocCanceled) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  const uint64_t id = book.add_limit_order_IOC(Side::BUY, 100, 5);

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].side, Side::BUY);
  EXPECT_EQ(ioc[0].canceled_quantity, 5u);
  EXPECT_FALSE(book.get_best_bid().has_value());
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookLimitIoc, NonCrossingBuyBelowAskEmitsIocAndLeavesBookUnchanged) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::SELL, 100, 4);
  const uint64_t id = book.add_limit_order_IOC(Side::BUY, 99, 3);

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].canceled_quantity, 3u);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 100u);
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookLimitIoc, PartialFillEmitsCanceledRemainderAndDoesNotRest) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::SELL, 100, 2);
  const uint64_t id = book.add_limit_order_IOC(Side::BUY, 100, 10);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 2u);
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].side, Side::BUY);
  EXPECT_EQ(ioc[0].canceled_quantity, 8u);
  EXPECT_FALSE(book.get_best_ask().has_value());
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookLimitIoc, FullyFilledOrderDoesNotEmitIocCanceled) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::SELL, 100, 5);
  book.add_limit_order_IOC(Side::BUY, 100, 5);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 5u);
  EXPECT_TRUE(ioc.empty());
  EXPECT_FALSE(book.get_best_ask().has_value());
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookLimitIoc, SellIntoEmptyBookEmitsFullIocCanceled) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  const uint64_t id = book.add_limit_order_IOC(Side::SELL, 100, 4);

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].side, Side::SELL);
  EXPECT_EQ(ioc[0].canceled_quantity, 4u);
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookLimitIoc, SellAboveBidEmitsIocAndLeavesBookUnchanged) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::BUY, 100, 6);
  const uint64_t id = book.add_limit_order_IOC(Side::SELL, 101, 2);

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].canceled_quantity, 2u);
  auto bid = book.get_best_bid();
  ASSERT_TRUE(bid);
  EXPECT_EQ(*bid, 100u);
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookLimitIoc, SellPartialFillEmitsCanceledRemainder) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::BUY, 100, 3);
  const uint64_t id = book.add_limit_order_IOC(Side::SELL, 100, 9);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 3u);
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].side, Side::SELL);
  EXPECT_EQ(ioc[0].canceled_quantity, 6u);
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookLimitIoc, BuyRespectsPriceBoundAndSkipsWorseAskLevels) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order(Side::SELL, 101, 1);
  const uint64_t id = book.add_limit_order_IOC(Side::BUY, 100, 2);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].price, 100u);
  EXPECT_EQ(trades[0].quantity, 1u);
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].canceled_quantity, 1u);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 101u);
}

TEST(OrderBookLimitIoc, SweepsMultipleLevelsWithinPriceBound) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::SELL, 100, 2);
  book.add_limit_order(Side::SELL, 101, 2);
  book.add_limit_order_IOC(Side::BUY, 101, 3);

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].price, 100u);
  EXPECT_EQ(trades[0].quantity, 2u);
  EXPECT_EQ(trades[1].price, 101u);
  EXPECT_EQ(trades[1].quantity, 1u);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 101u);
}

TEST(OrderBookLimitIoc, FifoAtSamePriceWithinBound) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  const uint64_t s1 = book.add_limit_order(Side::SELL, 100, 1);
  const uint64_t s2 = book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order_IOC(Side::BUY, 100, 2);

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].taker_order_id, s1);
  EXPECT_EQ(trades[1].taker_order_id, s2);
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookLimitIoc, CannotCancelBecauseOrderNeverRested) {
  OrderBook book;
  const uint64_t id = book.add_limit_order_IOC(Side::BUY, 100, 3);
  EXPECT_FALSE(book.cancel_order(id));
}

TEST(OrderBookLimitIoc, SetIocCallbackAfterConstructionIsUsed) {
  std::vector<IocCanceled> ioc;
  OrderBook book;
  book.set_ioc_canceled_callback(
      [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  const uint64_t id = book.add_limit_order_IOC(Side::SELL, 100, 2);

  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].canceled_quantity, 2u);
}

TEST(OrderBookLimitIoc, SequenceContinuesAcrossLimitIocThenLimitMatch) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order_IOC(Side::BUY, 100, 2);
  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].sequence, 0u);
  ASSERT_EQ(ioc.size(), 1u);

  book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order(Side::BUY, 100, 1);
  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[1].sequence, 1u);
}

// ---- limit FOK ----

TEST(OrderBookLimitFok, NonCrossingBuyIntoEmptyBookKillsEntireOrder) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  const uint64_t id = book.add_limit_order_FOK(Side::BUY, 100, 5);

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].side, Side::BUY);
  EXPECT_EQ(ioc[0].canceled_quantity, 5u);
  EXPECT_FALSE(book.get_best_bid().has_value());
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookLimitFok,
     NonCrossingBuyBelowAskKillsEntireOrderAndLeavesBookUnchanged) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::SELL, 100, 4);
  const uint64_t id = book.add_limit_order_FOK(Side::BUY, 99, 3);

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].canceled_quantity, 3u);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 100u);
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookLimitFok, PartialLiquidityKillsEntireOrderWithoutTrading) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::SELL, 100, 2);
  const uint64_t id = book.add_limit_order_FOK(Side::BUY, 100, 10);

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].side, Side::BUY);
  EXPECT_EQ(ioc[0].canceled_quantity, 10u);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 100u);
  EXPECT_EQ(book.get_best_bid(), std::nullopt);
}

TEST(OrderBookLimitFok, FullyFilledOrderDoesNotEmitIocCanceled) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::SELL, 100, 5);
  book.add_limit_order_FOK(Side::BUY, 100, 5);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 5u);
  EXPECT_TRUE(ioc.empty());
  EXPECT_FALSE(book.get_best_ask().has_value());
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookLimitFok, SellIntoEmptyBookKillsEntireOrder) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  const uint64_t id = book.add_limit_order_FOK(Side::SELL, 100, 4);

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].side, Side::SELL);
  EXPECT_EQ(ioc[0].canceled_quantity, 4u);
  EXPECT_FALSE(book.get_best_bid().has_value());
}

TEST(OrderBookLimitFok, SellAboveBidKillsEntireOrderAndLeavesBookUnchanged) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::BUY, 100, 6);
  const uint64_t id = book.add_limit_order_FOK(Side::SELL, 101, 2);

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].canceled_quantity, 2u);
  auto bid = book.get_best_bid();
  ASSERT_TRUE(bid);
  EXPECT_EQ(*bid, 100u);
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookLimitFok, SellPartialLiquidityKillsEntireOrderWithoutTrading) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::BUY, 100, 3);
  const uint64_t id = book.add_limit_order_FOK(Side::SELL, 100, 9);

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].side, Side::SELL);
  EXPECT_EQ(ioc[0].canceled_quantity, 9u);
  auto bid = book.get_best_bid();
  ASSERT_TRUE(bid);
  EXPECT_EQ(*bid, 100u);
}

TEST(OrderBookLimitFok, BuyRespectsPriceBoundWhenScanningLiquidity) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order(Side::SELL, 101, 5);
  const uint64_t id = book.add_limit_order_FOK(Side::BUY, 100, 2);

  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].canceled_quantity, 2u);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 100u);
}

TEST(OrderBookLimitFok, SweepsMultipleLevelsWhenFullyFillable) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::SELL, 100, 2);
  book.add_limit_order(Side::SELL, 101, 2);
  book.add_limit_order_FOK(Side::BUY, 101, 3);

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].price, 100u);
  EXPECT_EQ(trades[0].quantity, 2u);
  EXPECT_EQ(trades[1].price, 101u);
  EXPECT_EQ(trades[1].quantity, 1u);
  auto ask = book.get_best_ask();
  ASSERT_TRUE(ask);
  EXPECT_EQ(*ask, 101u);
}

TEST(OrderBookLimitFok, FifoAtSamePriceWhenFullyFillable) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  const uint64_t s1 = book.add_limit_order(Side::SELL, 100, 1);
  const uint64_t s2 = book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order_FOK(Side::BUY, 100, 2);

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].taker_order_id, s1);
  EXPECT_EQ(trades[1].taker_order_id, s2);
  EXPECT_FALSE(book.get_best_ask().has_value());
}

TEST(OrderBookLimitFok, CannotCancelBecauseOrderNeverRested) {
  OrderBook book;
  const uint64_t id = book.add_limit_order_FOK(Side::BUY, 100, 3);
  EXPECT_FALSE(book.cancel_order(id));
}

TEST(OrderBookLimitFok, SetIocCallbackAfterConstructionIsUsedOnKill) {
  std::vector<IocCanceled> ioc;
  OrderBook book;
  book.set_ioc_canceled_callback(
      [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  const uint64_t id = book.add_limit_order_FOK(Side::SELL, 100, 2);

  ASSERT_EQ(ioc.size(), 1u);
  EXPECT_EQ(ioc[0].order_id, id);
  EXPECT_EQ(ioc[0].canceled_quantity, 2u);
}

TEST(OrderBookLimitFok, SequenceContinuesAcrossKillThenLimitMatch) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order_FOK(Side::BUY, 100, 2);
  EXPECT_TRUE(trades.empty());
  ASSERT_EQ(ioc.size(), 1u);

  book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order(Side::BUY, 100, 1);
  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].sequence, 0u);
}

TEST(OrderBookLimitFok, FokDiffersFromIocOnPartialLiquidity) {
  std::vector<Trade> ioc_trades;
  std::vector<IocCanceled> ioc_canceled;
  OrderBook ioc_book(
      [&ioc_trades](const Trade &t) { ioc_trades.push_back(t); },
      [&ioc_canceled](const IocCanceled &c) { ioc_canceled.push_back(c); });

  std::vector<Trade> fok_trades;
  std::vector<IocCanceled> fok_canceled;
  OrderBook fok_book(
      [&fok_trades](const Trade &t) { fok_trades.push_back(t); },
      [&fok_canceled](const IocCanceled &c) { fok_canceled.push_back(c); });

  ioc_book.add_limit_order(Side::SELL, 100, 2);
  fok_book.add_limit_order(Side::SELL, 100, 2);

  ioc_book.add_limit_order_IOC(Side::BUY, 100, 5);
  fok_book.add_limit_order_FOK(Side::BUY, 100, 5);

  ASSERT_EQ(ioc_trades.size(), 1u);
  EXPECT_EQ(ioc_trades[0].quantity, 2u);
  ASSERT_EQ(ioc_canceled.size(), 1u);
  EXPECT_EQ(ioc_canceled[0].canceled_quantity, 3u);

  EXPECT_TRUE(fok_trades.empty());
  ASSERT_EQ(fok_canceled.size(), 1u);
  EXPECT_EQ(fok_canceled[0].canceled_quantity, 5u);
}

// ---- trade sequence ----

TEST(OrderBookTradeSequence, EmittedTradesHaveMonotonicSequence) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order(Side::BUY, 100, 2);

  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[0].sequence, 0u);
  EXPECT_EQ(trades[1].sequence, 1u);
}

TEST(OrderBookTradeSequence, SequenceContinuesAcrossMarketThenLimitMatch) {
  std::vector<Trade> trades;
  std::vector<IocCanceled> ioc;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); },
                 [&ioc](const IocCanceled &c) { ioc.push_back(c); });

  book.add_limit_order(Side::SELL, 100, 1);
  book.add_market_order(Side::BUY, 2);
  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].sequence, 0u);
  ASSERT_EQ(ioc.size(), 1u);

  book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order(Side::BUY, 100, 1);
  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[1].sequence, 1u);
}

TEST(OrderBookLimit,
     SelfCrossAttemptMatchesInCurrentModelWithoutOwnerIdentity) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  // Book has no account/owner identity, so "self-cross prevention" is out of
  // scope. This locks in current behavior: crossing orders always match.
  const uint64_t s1 = book.add_limit_order(Side::SELL, 100, 1);
  book.add_limit_order(Side::BUY, 100, 1);

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].taker_order_id, s1);
  EXPECT_EQ(trades[0].sequence, 0u);
}

// ---- pool / level reuse over many ops ----

TEST(OrderBookLimit, LargeCancelChurnDoesNotCorruptBook) {
  std::vector<Trade> trades;
  OrderBook book([&trades](const Trade &t) { trades.push_back(t); });

  // Add a thousand bids, cancel each, then trade through fresh asks.
  std::vector<uint64_t> ids;
  ids.reserve(1000);
  for (int i = 0; i < 1000; ++i) {
    ids.push_back(book.add_limit_order(Side::BUY, 100, 1));
  }
  for (uint64_t id : ids) {
    EXPECT_TRUE(book.cancel_order(id));
  }
  EXPECT_FALSE(book.get_best_bid().has_value());

  book.add_limit_order(Side::SELL, 100, 2);
  book.add_limit_order(Side::BUY, 100, 2);
  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].quantity, 2u);
}
