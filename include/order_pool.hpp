#pragma once

#include "order.hpp"
#include <cstddef>
#include <memory>
#include <vector>

class OrderPool {
public:
  OrderPool();
  ~OrderPool() = default;

  Order *allocate();
  void deallocate(Order *order);
  void grow();

private:
  std::vector<std::unique_ptr<Order[]>> chunks_;
  Order *free_head_ = nullptr;
  size_t chunk_size = 512;
};
