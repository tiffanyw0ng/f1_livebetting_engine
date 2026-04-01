#include "Replay.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace {
EventType parseEventType(const std::string& token) {
    static const std::unordered_map<std::string, EventType> kTypes = {
        {"START", EventType::START},
        {"ODDS", EventType::ODDS},
        {"LAP", EventType::LAP},
        {"OVERTAKE", EventType::OVERTAKE},
        {"PIT", EventType::PIT},
        {"DNF", EventType::DNF},
        {"BET", EventType::BET},
        {"FINISH", EventType::FINISH},
    };

    const auto it = kTypes.find(token);
    if (it == kTypes.end()) {
        throw std::runtime_error("Unknown event type: " + token);
    }
    return it->second;
}
}

std::vector<Event> Replay::loadFromFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to open replay file: " + path);
    }

    std::vector<Event> events;
    events.reserve(256);

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream lineStream(line);
        Event event;
        std::string typeToken;
        lineStream >> event.timestampMs >> typeToken;
        if (!lineStream) {
            throw std::runtime_error("Invalid replay line: " + line);
        }

        event.type = parseEventType(typeToken);

        std::string token;
        while (lineStream >> token) {
            event.tokens.push_back(token);
        }

        events.push_back(std::move(event));
    }

    std::sort(events.begin(), events.end(), [](const Event& lhs, const Event& rhs) {
        return lhs.timestampMs < rhs.timestampMs;
    });

    return events;
}
