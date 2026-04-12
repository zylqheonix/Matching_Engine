#pragma once

#include "order.hpp"
#include <functional>
#include <list>
#include <map>
#include <unordered_map>

struct Trade {
  uint64_t maker_order_id;
  uint64_t taker_order_id;
  uint64_t price;
  uint64_t quantity;
  uint64_t sequence;
};

struct LookupEntry {
  Side side;
  uint64_t price;
  std::list<Order>::iterator order_iterator;
};

class OrderBook {
public:
  void add_limit_order(const Order &order);
  Order get_best_ask();
  Order get_best_bid();

private:
  std::map<uint64_t, std::list<Order>, std::greater<uint64_t>> bids;
  std::map<uint64_t, std::list<Order>, std::less<uint64_t>> asks;
  std::unordered_map<uint64_t, LookupEntry> lookup_table;
};
