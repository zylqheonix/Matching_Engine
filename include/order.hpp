#pragma once
#include <cstdint>
#include <chrono>

enum class Side {
    BUY = 0,
    SELL = 1
};

struct Order {
    uint64_t id;
    Side side;
    double price;
    double quantity;
    std::chrono::system_clock::time_point timestamp;
    bool is_filled;
};