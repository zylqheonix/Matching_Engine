#include "order_book.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

OrderBook::OrderBook(std::function<void(const Trade &)> trade_callback,
                     std::function<void(const IocCanceled &)> ioc_canceled_callback)
    : trade_callback_(std::move(trade_callback)),
      ioc_canceled_callback_(std::move(ioc_canceled_callback)) {}

void OrderBook::set_trade_callback(
    std::function<void(const Trade &)> trade_callback) {
  trade_callback_ = std::move(trade_callback);
}

void OrderBook::set_ioc_canceled_callback(
    std::function<void(const IocCanceled &)> ioc_canceled_callback) {
  ioc_canceled_callback_ = std::move(ioc_canceled_callback);
}

std::optional<Order> OrderBook::get_best_ask() {
  if (asks.empty()) {
    return std::optional<Order>{};
  }
  return asks.begin()->second.front();
}

std::optional<Order> OrderBook::get_best_bid() {
  if (bids.empty()) {
    return std::optional<Order>{};
  }
  return bids.begin()->second.front();
}

void OrderBook::add_limit_order(const Order &order) {
  if (order.type != OrderType::LIMIT || !order.price.has_value()) {
    throw std::invalid_argument("limit order requires a limit price");
  }
  const uint64_t limit_price = *order.price;

  if (order.side == Side::BUY) {
    const uint64_t remaining_quantity = match_against_book<Side::BUY>(
        order, std::optional<uint64_t>(limit_price));
    if (remaining_quantity > 0) {
      auto &lst = bids[limit_price];
      Order resting = order;
      resting.quantity = remaining_quantity;
      lst.push_back(resting);
      lookup_table[order.id] = LookupEntry{
          Side::BUY,
          limit_price,
          std::prev(lst.end()),
      };
    }
  } else {
    const uint64_t remaining_quantity = match_against_book<Side::SELL>(
        order, std::optional<uint64_t>(limit_price));
    if (remaining_quantity > 0) {
      auto &lst = asks[limit_price];
      Order resting = order;
      resting.quantity = remaining_quantity;
      lst.push_back(resting);
      lookup_table[order.id] = LookupEntry{
          Side::SELL,
          limit_price,
          std::prev(lst.end()),
      };
    }
  }
}

void OrderBook::add_market_order(const Order &order) {
  if (order.type != OrderType::MARKET) {
    throw std::invalid_argument("market order must have market type");
  }

  const std::optional<uint64_t> no_limit;
  uint64_t remaining_quantity = 0;
  if (order.side == Side::BUY) {
    remaining_quantity = match_against_book<Side::BUY>(order, no_limit);
  } else {
    remaining_quantity = match_against_book<Side::SELL>(order, no_limit);
  }

  if (remaining_quantity > 0) {
    ioc_canceled_callback_(
        IocCanceled{order.id, order.side, remaining_quantity});
  }
}

bool OrderBook::cancel_order(const uint64_t &order_id) {
  auto lookup_it = lookup_table.find(order_id);
  if (lookup_it == lookup_table.end()) {
    return false;
  }

  const LookupEntry &entry = lookup_it->second;
  if (entry.side == Side::BUY) {
    auto price_level_it = bids.find(entry.price);
    if (price_level_it == bids.end()) {
      return false;
    }

    price_level_it->second.erase(entry.order_iterator);
    if (price_level_it->second.empty()) {
      bids.erase(price_level_it);
    }
  } else {
    auto price_level_it = asks.find(entry.price);
    if (price_level_it == asks.end()) {
      return false;
    }

    price_level_it->second.erase(entry.order_iterator);
    if (price_level_it->second.empty()) {
      asks.erase(price_level_it);
    }
  }
  lookup_table.erase(lookup_it);
  return true;
}

template <Side taker_side>
uint64_t OrderBook::match_against_book(
    const Order &taker,
    const std::optional<uint64_t> &limit_price_constraint) {
  uint64_t remaining_quantity = taker.quantity;

  if constexpr (taker_side == Side::BUY) {
    while (!asks.empty() && remaining_quantity > 0) {
      const uint64_t best_ask_px = asks.begin()->first;
      if (limit_price_constraint.has_value() &&
          best_ask_px > *limit_price_constraint) {
        break;
      }
      auto &[price, level] = *asks.begin();

      while (remaining_quantity > 0 && !level.empty()) {
        Order &maker = level.front();
        const uint64_t fill =
            std::min<uint64_t>(remaining_quantity, maker.quantity);
        remaining_quantity -= fill;
        maker.quantity -= fill;

        Trade trade = {taker.id, maker.id, price, fill, next_trade_sequence_++};
        trade_callback_(trade);

        if (maker.quantity == 0) {
          lookup_table.erase(maker.id);
          level.pop_front();
        }
      }

      if (level.empty()) {
        asks.erase(price);
      }
    }
  } else {
    while (!bids.empty() && remaining_quantity > 0) {
      const uint64_t best_bid_px = bids.begin()->first;
      if (limit_price_constraint.has_value() &&
          best_bid_px < *limit_price_constraint) {
        break;
      }
      auto &[price, level] = *bids.begin();

      while (remaining_quantity > 0 && !level.empty()) {
        Order &maker = level.front();
        const uint64_t fill =
            std::min<uint64_t>(remaining_quantity, maker.quantity);
        remaining_quantity -= fill;
        maker.quantity -= fill;

        Trade trade = {taker.id, maker.id, price, fill, next_trade_sequence_++};
        trade_callback_(trade);

        if (maker.quantity == 0) {
          lookup_table.erase(maker.id);
          level.pop_front();
        }
      }

      if (level.empty()) {
        bids.erase(price);
      }
    }
  }

  return remaining_quantity;
}

template uint64_t OrderBook::match_against_book<Side::BUY>(
    const Order &, const std::optional<uint64_t> &);
template uint64_t OrderBook::match_against_book<Side::SELL>(
    const Order &, const std::optional<uint64_t> &);
