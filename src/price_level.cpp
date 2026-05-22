#include "price_level.hpp"

Order *PriceLevel::front() const { return head_; }

void PriceLevel::push_back(Order *order) {
  if (order == nullptr) {
    return;
  }

  order->next = order->prev = nullptr;

  if (head_ == nullptr) {
    head_ = tail_ = order;
  } else {
    order->prev = tail_;
    tail_->next = order;
    tail_ = order;
  }
  order->next = nullptr;
}

Order *PriceLevel::pop_front() {
  if (head_ == nullptr) {
    return nullptr;
  }

  Order *order = head_;
  head_ = head_->next;
  if (head_ == nullptr) {
    tail_ = nullptr;
  } else {
    head_->prev = nullptr;
  }
  order->next = order->prev = nullptr;
  return order;
}

void PriceLevel::erase(Order *order) {
  if (order == nullptr) {
    return;
  }

  if (order->prev != nullptr) {
    order->prev->next = order->next;
  } else {
    head_ = order->next;
  }

  if (order->next != nullptr) {
    order->next->prev = order->prev;
  } else {
    tail_ = order->prev;
  }

  order->next = order->prev = nullptr;
}
