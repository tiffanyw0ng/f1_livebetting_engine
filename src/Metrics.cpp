#include "Metrics.h"

#include <algorithm>
#include <iomanip>

void Metrics::startRun() {
    runStart_ = std::chrono::steady_clock::now();
}

void Metrics::finishRun() {
    runEnd_ = std::chrono::steady_clock::now();
}

void Metrics::recordEventLatency(long long latencyNs) {
    ++totalEvents_;
    totalLatencyNs_ += latencyNs;
    maxLatencyNs_ = std::max(maxLatencyNs_, latencyNs);
}

void Metrics::recordTrade() {
    ++totalTrades_;
}

void Metrics::print(std::ostream& out) const {
    const auto runtimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(runEnd_ - runStart_).count();
    const double avgLatency = totalEvents_ == 0 ? 0.0 : static_cast<double>(totalLatencyNs_) / static_cast<double>(totalEvents_);
    const double throughput = runtimeNs <= 0
        ? 0.0
        : (static_cast<double>(totalEvents_) * 1'000'000'000.0) / static_cast<double>(runtimeNs);

    out << "--- Metrics ---\n";
    out << "Events processed: " << totalEvents_ << '\n';
    out << "Trades matched: " << totalTrades_ << '\n';
    out << std::fixed << std::setprecision(2);
    out << "Avg latency (ns): " << avgLatency << '\n';
    out << "Max latency (ns): " << maxLatencyNs_ << '\n';
    out << "Throughput (events/sec): " << throughput << '\n';
}
