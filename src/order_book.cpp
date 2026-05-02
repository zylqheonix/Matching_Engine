#include "order_book.hpp"
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

OrderBook::OrderBook(std::function<void(const Trade &)> trade_callback,
                     std::function<void(const IocCanceled &)> ioc_canceled_callback)
    : trade_callback_(std::move(trade_callback)),
      ioc_canceled_callback_(std::move(ioc_canceled_callback)) {}

void OrderBook::set_trade_callback(
    std::function<void(const Trade &)> trade_callback) {
  this->trade_callback_ = std::move(trade_callback);
}

void OrderBook::set_ioc_canceled_callback(
    std::function<void(const IocCanceled &)> ioc_canceled_callback) {
  this->ioc_canceled_callback_ = std::move(ioc_canceled_callback);
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
    uint64_t remaining_quantity = order.quantity;
    while (!this->asks.empty() && remaining_quantity > 0 &&
           this->asks.begin()->first <= limit_price) {
      auto &[price, ask] = *this->asks.begin();

      while (remaining_quantity > 0 && !ask.empty()) {

        Order &ask_order = ask.front();
        uint64_t fill =
            std::min<uint64_t>(remaining_quantity, ask_order.quantity);
        remaining_quantity -= fill;
        ask_order.quantity -= fill;

        Trade trade = {order.id, ask_order.id, price, fill, 0};
        this->trade_callback_(trade);

        if (ask_order.quantity == 0) {
          lookup_table.erase(ask_order.id);
          ask.pop_front();
        }
      }

      if (ask.empty()) {
        this->asks.erase(price);
      }
    }

    if (remaining_quantity > 0) {
      auto &lst = this->bids[limit_price];
      Order resting = order;
      resting.quantity = remaining_quantity;
      lst.push_back(resting);
      this->lookup_table[order.id] = LookupEntry{
          Side::BUY,
          limit_price,
          std::prev(lst.end()),
      };
    }
  } else {

    uint64_t remaining_quantity = order.quantity;
    while (!this->bids.empty() && remaining_quantity > 0 &&
           this->bids.begin()->first >= limit_price) {
      auto &[price, bid] = *this->bids.begin();

      while (remaining_quantity > 0 && !bid.empty()) {
        Order &bid_order = bid.front();
        uint64_t fill =
            std::min<uint64_t>(remaining_quantity, bid_order.quantity);
        remaining_quantity -= fill;
        bid_order.quantity -= fill;
        Trade trade = {order.id, bid_order.id, price, fill, 0};
        this->trade_callback_(trade);
        if (bid_order.quantity == 0) {
          lookup_table.erase(bid_order.id);
          bid.pop_front();
        }
      }
      if (bid.empty()) {
        this->bids.erase(price);
      }
    }

    if (remaining_quantity > 0) {
      auto &lst = this->asks[limit_price];
      Order resting = order;
      resting.quantity = remaining_quantity;
      lst.push_back(resting);
      this->lookup_table[order.id] = LookupEntry{
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

  uint64_t remaining_quantity = order.quantity;
  if (order.side == Side::BUY) {
    while (!this->asks.empty() && remaining_quantity > 0) {
      auto &[price, ask] = *this->asks.begin();

      while (remaining_quantity > 0 && !ask.empty()) {
        Order &ask_order = ask.front();
        const uint64_t fill =
            std::min<uint64_t>(remaining_quantity, ask_order.quantity);
        remaining_quantity -= fill;
        ask_order.quantity -= fill;

        Trade trade = {order.id, ask_order.id, price, fill, 0};
        this->trade_callback_(trade);

        if (ask_order.quantity == 0) {
          lookup_table.erase(ask_order.id);
          ask.pop_front();
        }
      }

      if (ask.empty()) {
        this->asks.erase(price);
      }
    }
  } else {
    while (!this->bids.empty() && remaining_quantity > 0) {
      auto &[price, bid] = *this->bids.begin();

      while (remaining_quantity > 0 && !bid.empty()) {
        Order &bid_order = bid.front();
        const uint64_t fill =
            std::min<uint64_t>(remaining_quantity, bid_order.quantity);
        remaining_quantity -= fill;
        bid_order.quantity -= fill;

        Trade trade = {order.id, bid_order.id, price, fill, 0};
        this->trade_callback_(trade);

        if (bid_order.quantity == 0) {
          lookup_table.erase(bid_order.id);
          bid.pop_front();
        }
      }

      if (bid.empty()) {
        this->bids.erase(price);
      }
    }
  }

  if (remaining_quantity > 0) {
    this->ioc_canceled_callback_(
        IocCanceled{order.id, order.side, remaining_quantity});
  }
}

bool OrderBook::cancel_order(const uint64_t &order_id) {
  auto lookup_it = this->lookup_table.find(order_id);
  if (lookup_it == this->lookup_table.end()) {
    return false;
  }

  const LookupEntry &entry = lookup_it->second;
  if (entry.side == Side::BUY) {
    auto price_level_it = this->bids.find(entry.price);
    if (price_level_it == this->bids.end()) {
      return false;
    }

    price_level_it->second.erase(entry.order_iterator);
    if (price_level_it->second.empty()) {
      this->bids.erase(price_level_it);
    }
  } else {
    auto price_level_it = this->asks.find(entry.price);
    if (price_level_it == this->asks.end()) {
      return false;
    }

    price_level_it->second.erase(entry.order_iterator);
    if (price_level_it->second.empty()) {
      this->asks.erase(price_level_it);
    }
  }
  this->lookup_table.erase(lookup_it);
  return true;
}
