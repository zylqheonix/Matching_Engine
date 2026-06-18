#include "order_book.hpp"
#include <algorithm>
#include <cstdint>
#include <utility>

OrderBook::OrderBook(
    std::function<void(const Trade &)> trade_callback,
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

std::optional<uint64_t> OrderBook::get_best_ask() {
  if (asks.empty()) {
    return std::optional<uint64_t>{};
  }
  return std::optional<uint64_t>{asks.begin()->first};
}

std::optional<uint64_t> OrderBook::get_best_bid() {
  if (bids.empty()) {
    return std::optional<uint64_t>{};
  }
  return std::optional<uint64_t>{bids.begin()->first};
}

uint64_t
OrderBook::add_limit_order(Side side, uint64_t price, uint64_t quantity) {

  const uint64_t order_id = ++next_order_id_;
  Order incoming{
      .id = order_id,
      .type = OrderType::LIMIT,
      .side = side,
      .quantity = quantity,
      .price = price,
      .timestamp = std::chrono::system_clock::now(),
  };

  Order *resting = nullptr;
  uint64_t remaining_quantity = 0;
  if (side == Side::BUY) {
    remaining_quantity =
        match_against_book<Side::BUY>(incoming, std::optional<uint64_t>(price));
    if (remaining_quantity > 0) {
      resting = order_pool_.allocate();
      *resting = incoming;
      resting->quantity = remaining_quantity;
      auto [it, _] = bids.try_emplace(price, price);
      it->second.push_back(resting);
    }
  } else {
    remaining_quantity = match_against_book<Side::SELL>(
        incoming, std::optional<uint64_t>(price));
    if (remaining_quantity > 0) {
      resting = order_pool_.allocate();
      *resting = incoming;
      resting->quantity = remaining_quantity;
      auto [it, _] = asks.try_emplace(price, price);
      it->second.push_back(resting);
    }
  }

  if (resting != nullptr) {
    lookup_table[incoming.id] = LookupEntry{side, price, resting};
  }

  return order_id;
}

uint64_t
OrderBook::add_limit_order_IOC(Side side, uint64_t price, uint64_t quantity) {

  Order incoming{
      .id = ++next_order_id_,
      .type = OrderType::LIMIT,
      .side = side,
      .quantity = quantity,
      .price = price,
      .timestamp = std::chrono::system_clock::now(),
  };
  uint64_t remaining_quantity = 0;
  if (side == Side::BUY) {
    remaining_quantity =
        match_against_book<Side::BUY>(incoming, std::optional<uint64_t>(price));
  } else {
    remaining_quantity = match_against_book<Side::SELL>(
        incoming, std::optional<uint64_t>(price));
  }

  if (remaining_quantity > 0) {
    ioc_canceled_callback_(
        IocCanceled{incoming.id, incoming.side, remaining_quantity});
  }

  return incoming.id;
}

uint64_t OrderBook::add_market_order(Side side, uint64_t quantity) {

  const uint64_t order_id = ++next_order_id_;
  Order incoming{
      .id = order_id,
      .type = OrderType::MARKET,
      .side = side,
      .quantity = quantity,
      .timestamp = std::chrono::system_clock::now(),
  };

  const std::optional<uint64_t> no_limit;

  uint64_t remaining_quantity = 0;

  if (incoming.side == Side::BUY) {

    remaining_quantity = match_against_book<Side::BUY>(incoming, no_limit);

  } else {

    remaining_quantity = match_against_book<Side::SELL>(incoming, no_limit);
  }

  if (remaining_quantity > 0) {
    ioc_canceled_callback_(
        IocCanceled{incoming.id, incoming.side, remaining_quantity});
  }

  return order_id;
}

uint64_t
OrderBook::add_limit_order_FOK(Side side, uint64_t price, uint64_t quantity) {
  const uint64_t order_id = ++next_order_id_;
  Order incoming{
      .id = order_id,
      .type = OrderType::LIMIT,
      .side = side,
      .quantity = quantity,
      .price = price,
      .timestamp = std::chrono::system_clock::now(),
  };

  const auto scan_available = [&](const auto &book,
                                  auto should_stop) -> uint64_t {
    uint64_t available = 0;
    for (const auto &[order_price, level] : book) {
      if (should_stop(order_price)) {
        break;
      }
      for (Order *maker = level.front(); maker != nullptr;
           maker = maker->next) {
        available += maker->quantity;
        if (available >= quantity) {
          return available;
        }
      }
    }
    return available;
  };

  const uint64_t available_quantity =
      (side == Side::BUY)
          ? scan_available(
                asks, [&](uint64_t order_price) { return order_price > price; })
          : scan_available(bids, [&](uint64_t order_price) {
              return order_price < price;
            });

  if (available_quantity < quantity) {
    ioc_canceled_callback_(IocCanceled{incoming.id, incoming.side, quantity});
    return order_id;
  }

  const uint64_t remaining_quantity =
      (side == Side::BUY)
          ? match_against_book<Side::BUY>(incoming,
                                          std::optional<uint64_t>(price))
          : match_against_book<Side::SELL>(incoming,
                                           std::optional<uint64_t>(price));

  if (remaining_quantity > 0) {
    ioc_canceled_callback_(
        IocCanceled{incoming.id, incoming.side, remaining_quantity});
  }

  return order_id;
}

bool OrderBook::cancel_order(uint64_t order_id) {
  auto lookup_it = lookup_table.find(order_id);
  if (lookup_it == lookup_table.end()) {
    return false;
  }
  auto &entry = lookup_it->second;
  if (entry.side == Side::BUY) {
    auto price_level_it = bids.find(entry.price);

    if (price_level_it == bids.end())
      return false;
    auto &price_level = price_level_it->second;

    price_level.erase(entry.order_ptr);
    if (price_level.empty()) {
      bids.erase(price_level_it);
    }
  } else {
    auto price_level_it = asks.find(entry.price);
    if (price_level_it == asks.end())
      return false;
    auto &price_level = price_level_it->second;
    price_level.erase(entry.order_ptr);
    if (price_level.empty())
      asks.erase(price_level_it);
  }
  order_pool_.deallocate(entry.order_ptr);
  lookup_table.erase(lookup_it);
  return true;
}

template <Side taker_side>
uint64_t OrderBook::match_against_book(
    const Order &incoming,
    const std::optional<uint64_t> &limit_price_constraint) {

  uint64_t remaining_quantity = incoming.quantity;

  if constexpr (taker_side == Side::BUY) {
    while (!asks.empty() && remaining_quantity > 0) {
      const uint64_t best_ask_price = asks.begin()->first;
      if (limit_price_constraint.has_value() &&
          best_ask_price > *limit_price_constraint) {
        break;
      }
      auto &[price, level] = *asks.begin();

      while (remaining_quantity > 0 && !level.empty()) {
        Order *maker = level.front();
        const uint64_t fill =
            std::min<uint64_t>(remaining_quantity, maker->quantity);
        remaining_quantity -= fill;
        maker->quantity -= fill;

        Trade trade = {
            incoming.id, maker->id, price, fill, next_trade_sequence_++};
        trade_callback_(trade);

        if (maker->quantity == 0) {
          lookup_table.erase(maker->id);
          order_pool_.deallocate(level.pop_front());
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
        Order *maker = level.front();
        const uint64_t fill =
            std::min<uint64_t>(remaining_quantity, maker->quantity);
        remaining_quantity -= fill;
        maker->quantity -= fill;

        Trade trade = {
            incoming.id, maker->id, price, fill, next_trade_sequence_++};
        trade_callback_(trade);

        if (maker->quantity == 0) {
          lookup_table.erase(maker->id);
          order_pool_.deallocate(level.pop_front());
        }
      }

      if (level.empty()) {
        bids.erase(price);
      }
    }
  }

  return remaining_quantity;
}

template uint64_t
OrderBook::match_against_book<Side::BUY>(const Order &,
                                         const std::optional<uint64_t> &);
template uint64_t
OrderBook::match_against_book<Side::SELL>(const Order &,
                                          const std::optional<uint64_t> &);
