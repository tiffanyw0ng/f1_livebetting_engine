#pragma once

#include <chrono>
#include <cstddef>
#include <iostream>

class Metrics {
public:
    void startRun();
    void finishRun();
    void recordEventLatency(long long latencyNs);
    void recordTrade();
    void print(std::ostream& out) const;

private:
    std::size_t totalEvents_ = 0;
    std::size_t totalTrades_ = 0;
    long long totalLatencyNs_ = 0;
    long long maxLatencyNs_ = 0;
    std::chrono::steady_clock::time_point runStart_{};
    std::chrono::steady_clock::time_point runEnd_{};
};
