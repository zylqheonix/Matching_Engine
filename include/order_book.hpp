#pragma once
#include "order.hpp"
#include <vector>


class OrderBook {
    public:
    
    void add_order(const Order& order) {
        orders.push_back(order);
    }

    Order get_latest_order() {
        return orders.back();
    }

    private:
    std::vector<Order> orders;

};

