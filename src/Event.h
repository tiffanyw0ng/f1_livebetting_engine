#pragma once

#include <string>
#include <vector>

enum class EventType {
    START,
    ODDS,
    LAP,
    OVERTAKE,
    PIT,
    DNF,
    BET,
    FINISH
};

enum class Side {
    BACK,
    LAY
};

struct Event {
    long long timestampMs = 0;
    EventType type = EventType::START;
    std::vector<std::string> tokens;
};
