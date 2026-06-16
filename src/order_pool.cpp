#include "order_pool.hpp"

OrderPool::OrderPool() {
    grow();
}

void OrderPool::grow() {

    auto& slab = chunks_.emplace_back(std::make_unique<Order[]>(chunk_size));

    for (size_t i = 0; i < chunk_size; ++i) {
        slab[i].next = free_head_;
        free_head_ = &slab[i];
    }


}

Order* OrderPool::allocate() {
  
    if(free_head_ == nullptr) grow();
    Order* order = free_head_;
    free_head_ = order->next;
    order->next = nullptr;
    return order;

}

void OrderPool::deallocate(Order* order) {
    order->next = free_head_;
    free_head_ = order;
}

