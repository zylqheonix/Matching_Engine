#pragma once

#include "order.hpp"
#include <cstdint>

/// Intrusive FIFO queue per price tick (orders linked via Order::prev /
/// Order::next).
class PriceLevel {
public:
  explicit PriceLevel(uint64_t price) : price_(price) {}

  [[nodiscard]] uint64_t price() const { return price_; }

  void push_back(Order *order);
  [[nodiscard]] Order *pop_front();
  [[nodiscard]] bool empty() const { return head_ == nullptr; }
  [[nodiscard]] Order *front() const;
  void erase(Order *order);

private:
  uint64_t price_;
  Order *head_ = nullptr;
  Order *tail_ = nullptr;
};
