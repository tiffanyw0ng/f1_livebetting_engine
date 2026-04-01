#pragma once

#include "Event.h"
#include "Market.h"
#include "Metrics.h"
#include "Order.h"

#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

class Engine {
public:
    explicit Engine(std::ostream& out);

    void process(const std::vector<Event>& events);

private:
    struct RaceState {
        std::vector<std::string> runningOrder;
        std::vector<std::string> finishOrder;
    };

    void processEvent(const Event& event);
    void handleStart(const Event& event);
    void handleOddsEvent(const Event& event);
    void handleLapEvent(const Event& event);
    void handleOvertakeEvent(const Event& event);
    void handlePitEvent(const Event& event);
    void handleDnfEvent(const Event& event);
    void handleBetEvent(const Event& event);
    void handleFinishEvent(const Event& event);

    void repricing(const std::string& reason, long long timestampMs);
    void applyPositionBonuses();
    void matchOrder(Order&& incoming);
    void addRestingOrder(Order&& order);
    void logTrade(long long timestampMs, const std::string& driver, double tradeOdds, int quantity,
                  const std::string& backUser, const std::string& layUser);
    double scoreToOdds(const DriverBook& book) const;
    int driverBaseScore(std::size_t index) const;
    DriverBook& requireBook(const std::string& driver);
    const DriverBook& requireBook(const std::string& driver) const;
    static OrderSide parseOrderSide(const std::string& sideToken);
    static const char* toString(OrderSide side);

    std::ostream& out_;
    Metrics metrics_;
    RaceState raceState_;
    std::unordered_map<std::string, DriverBook> books_;
    long long nextOrderId_ = 1;
};
