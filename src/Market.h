#pragma once

#include "Order.h"

#include <deque>
#include <functional>
#include <map>

struct DriverBook {
    std::map<double, std::deque<Order>, std::greater<double>> backs;
    std::map<double, std::deque<Order>> lays;
    double currentOdds = 0.0;
    bool active = true;
    int score = 0;
    double latestLapSeconds = 0.0;
};
