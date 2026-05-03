#pragma once

#include "order.hpp"
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>

struct Trade {
  uint64_t maker_order_id;
  uint64_t taker_order_id;
  uint64_t price;
  uint64_t quantity;
  uint64_t sequence;
};

/// Emitted when a market order (IOC) finishes with size that did not rest.
struct IocCanceled {
  uint64_t order_id;
  Side side;
  uint64_t canceled_quantity;
};

struct LookupEntry {
  Side side;
  uint64_t price;
  std::list<Order>::iterator order_iterator;
};

class OrderBook {
public:
  explicit OrderBook(
      std::function<void(const Trade &)> trade_callback = [](const Trade &) {},
      std::function<void(const IocCanceled &)> ioc_canceled_callback =
          [](const IocCanceled &) {});

  void set_trade_callback(std::function<void(const Trade &)> trade_callback);
  void set_ioc_canceled_callback(
      std::function<void(const IocCanceled &)> ioc_canceled_callback);
  void add_limit_order(const Order &order);
  bool cancel_order(const uint64_t &order_id);
  /// Market orders use IOC: match now; unfilled size is canceled (see `IocCanceled`).
  void add_market_order(const Order &order);
  std::optional<Order> get_best_ask();
  std::optional<Order> get_best_bid();

private:
  friend struct OrderBookTestAccess;
  std::function<void(const Trade &)> trade_callback_;
  std::function<void(const IocCanceled &)> ioc_canceled_callback_;
  uint64_t next_trade_sequence_ = 0;
  std::map<uint64_t, std::list<Order>, std::greater<uint64_t>> bids;
  std::map<uint64_t, std::list<Order>, std::less<uint64_t>> asks;
  std::unordered_map<uint64_t, LookupEntry> lookup_table;

  /// Match aggressive `taker` against the opposite book. Returns unfilled quantity.
  /// `limit_price_constraint`: for limit orders, max trade price (buy) / min (sell);
  /// `nullopt` means market (no price bound).
  template <Side taker_side>
  uint64_t match_against_book(const Order &taker,
                              const std::optional<uint64_t> &limit_price_constraint);
};
