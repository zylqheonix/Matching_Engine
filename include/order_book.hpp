#pragma once

#include "order.hpp"
#include <functional>
#include <list>
#include <map>
#include <unordered_map>

struct LookupEntry {
  Side side;
  uint64_t price;
  std::list<Order>::iterator order_iterator;
};

class OrderBook {

private:
  std::map<uint64_t, std::list<Order>, std::greater<uint64_t>> bids;
  std::map<uint64_t, std::list<Order>, std::less<uint64_t>> asks;
  std::unordered_map<uint64_t, LookupEntry> lookup_table;
};
