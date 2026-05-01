#pragma once
#include <chrono>
#include <cstdint>
#include <optional>

enum class Side {
  BUY = 0,
  SELL = 1
};

enum class OrderType {
  LIMIT = 0,
  MARKET = 1
};

struct Order {
  uint64_t id;
  OrderType type = OrderType::LIMIT;
  Side side;
  uint64_t quantity;
  std::optional<uint64_t> price;
  std::chrono::system_clock::time_point timestamp;
};