#pragma once
#include "order_pool.hpp"
#include "order.hpp"
#include "price_level.hpp"
#include <cstdint>
#include <functional>
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

/// Emitted when an IOC order (market or limit) finishes with size that did not rest.
struct IocCanceled {
  uint64_t order_id;
  Side side;
  uint64_t canceled_quantity;
};

struct LookupEntry {
  Side side;
  uint64_t price;
  Order* order_ptr;
  
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
    /// Returns the engine-assigned order id (1-based).
    uint64_t add_limit_order(Side side, uint64_t price, uint64_t quantity);
    uint64_t add_limit_order_IOC(Side side, uint64_t price, uint64_t quantity);
    uint64_t add_limit_order_FOK(Side side, uint64_t price, uint64_t quantity);
    bool cancel_order(uint64_t order_id);
    /// Market orders use IOC: match now; unfilled size is canceled (see `IocCanceled`).
    /// Returns the engine-assigned order id (1-based).
    uint64_t add_market_order(Side side, uint64_t quantity);
    std::optional<uint64_t> get_best_ask();
    std::optional<uint64_t> get_best_bid();

  private:
    uint64_t next_order_id_ = 0;
    friend struct OrderBookTestAccess;
    std::function<void(const Trade &)> trade_callback_;
    std::function<void(const IocCanceled &)> ioc_canceled_callback_;
    uint64_t next_trade_sequence_ = 0;
    std::map<uint64_t, PriceLevel, std::greater<uint64_t>> bids;
    std::map<uint64_t, PriceLevel, std::less<uint64_t>> asks;
    std::unordered_map<uint64_t, LookupEntry> lookup_table;
    OrderPool order_pool_;
    /// Match aggressive `taker` against the opposite book. Returns unfilled quantity.
    /// `limit_price_constraint`: for limit orders, max trade price (buy) / min (sell);
    /// `nullopt` means market (no price bound).
    template <Side taker_side>
    uint64_t match_against_book(const Order& incoming,
                                const std::optional<uint64_t> &limit_price_constraint);
};
