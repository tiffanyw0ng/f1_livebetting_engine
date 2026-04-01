#pragma once

#include <string>

enum class OrderSide {
    BACK,
    LAY
};

struct Order {
    long long id = 0;
    std::string user;
    std::string driver;
    OrderSide side = OrderSide::BACK;
    double odds = 0.0;
    int quantity = 0;
    long long timestampMs = 0;
};
