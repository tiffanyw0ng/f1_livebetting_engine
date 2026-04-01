#include "Engine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
std::string formatDouble(double value, int precision = 2) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}
}

Engine::Engine(std::ostream& out) : out_(out) {}

void Engine::process(const std::vector<Event>& events) {
    metrics_.startRun();

    for (const Event& event : events) {
        const auto start = std::chrono::steady_clock::now();
        processEvent(event);
        const auto end = std::chrono::steady_clock::now();
        const auto latencyNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        metrics_.recordEventLatency(latencyNs);
    }

    metrics_.finishRun();
    metrics_.print(out_);
}

void Engine::processEvent(const Event& event) {
    switch (event.type) {
    case EventType::START:
        handleStart(event);
        break;
    case EventType::ODDS:
        handleOddsEvent(event);
        break;
    case EventType::LAP:
        handleLapEvent(event);
        break;
    case EventType::OVERTAKE:
        handleOvertakeEvent(event);
        break;
    case EventType::PIT:
        handlePitEvent(event);
        break;
    case EventType::DNF:
        handleDnfEvent(event);
        break;
    case EventType::BET:
        handleBetEvent(event);
        break;
    case EventType::FINISH:
        handleFinishEvent(event);
        break;
    }
}

void Engine::handleStart(const Event& event) {
    raceState_.runningOrder = event.tokens;
    raceState_.finishOrder.clear();
    books_.clear();
    books_.reserve(event.tokens.size());

    for (std::size_t index = 0; index < event.tokens.size(); ++index) {
        DriverBook book;
        book.score = driverBaseScore(index);
        book.currentOdds = scoreToOdds(book);
        books_.emplace(event.tokens[index], std::move(book));
    }

    out_ << '[' << event.timestampMs << "] START:";
    for (const std::string& driver : raceState_.runningOrder) {
        out_ << ' ' << driver;
    }
    out_ << '\n';
    repricing("grid initialized", event.timestampMs);
}

void Engine::handleOddsEvent(const Event& event) {
    if (event.tokens.size() != 2) {
        throw std::runtime_error("ODDS event expects: DRIVER ODDS");
    }

    const std::string& driver = event.tokens[0];
    DriverBook& book = requireBook(driver);
    book.currentOdds = std::stod(event.tokens[1]);

    out_ << '[' << event.timestampMs << "] WINNER odds updated: " << driver << ' '
         << formatDouble(book.currentOdds) << '\n';
}

void Engine::handleLapEvent(const Event& event) {
    if (event.tokens.size() != 3) {
        throw std::runtime_error("LAP event expects: DRIVER LAP_NUMBER LAP_TIME");
    }

    const std::string& driver = event.tokens[0];
    DriverBook& book = requireBook(driver);
    const double lapTime = std::stod(event.tokens[2]);
    book.latestLapSeconds = lapTime;

    const bool strongLap = lapTime < 92.5;
    if (strongLap && book.active) {
        book.score += 1;
    }

    out_ << '[' << event.timestampMs << "] LAP: " << driver << " lap " << event.tokens[1]
         << " in " << formatDouble(lapTime) << "s\n";

    if (strongLap) {
        repricing("strong lap by " + driver, event.timestampMs);
    }
}

void Engine::handleOvertakeEvent(const Event& event) {
    if (event.tokens.size() != 2) {
        throw std::runtime_error("OVERTAKE event expects: PASSER PASSED");
    }

    const std::string& passer = event.tokens[0];
    const std::string& passed = event.tokens[1];
    DriverBook& passerBook = requireBook(passer);
    DriverBook& passedBook = requireBook(passed);

    auto passerIt = std::find(raceState_.runningOrder.begin(), raceState_.runningOrder.end(), passer);
    auto passedIt = std::find(raceState_.runningOrder.begin(), raceState_.runningOrder.end(), passed);
    if (passerIt != raceState_.runningOrder.end() && passedIt != raceState_.runningOrder.end() && passerIt > passedIt) {
        std::iter_swap(passerIt, passedIt);
    }

    passerBook.score += 5;
    passedBook.score -= 4;

    out_ << '[' << event.timestampMs << "] OVERTAKE: " << passer << " over " << passed << '\n';
    repricing("overtake event", event.timestampMs);
}

void Engine::handlePitEvent(const Event& event) {
    if (event.tokens.size() != 1) {
        throw std::runtime_error("PIT event expects: DRIVER");
    }

    const std::string& driver = event.tokens[0];
    DriverBook& book = requireBook(driver);
    book.score -= 3;

    out_ << '[' << event.timestampMs << "] PIT: " << driver << '\n';
    repricing("pit stop by " + driver, event.timestampMs);
}

void Engine::handleDnfEvent(const Event& event) {
    if (event.tokens.size() != 1) {
        throw std::runtime_error("DNF event expects: DRIVER");
    }

    const std::string& driver = event.tokens[0];
    DriverBook& book = requireBook(driver);
    book.active = false;
    book.score = -1000;
    book.currentOdds = 999.0;

    raceState_.runningOrder.erase(
        std::remove(raceState_.runningOrder.begin(), raceState_.runningOrder.end(), driver),
        raceState_.runningOrder.end());

    out_ << '[' << event.timestampMs << "] DNF: " << driver << '\n';
    repricing("dnf for " + driver, event.timestampMs);
}

void Engine::handleBetEvent(const Event& event) {
    if (event.tokens.size() != 5) {
        throw std::runtime_error("BET event expects: USER SIDE DRIVER ODDS QTY");
    }

    Order order;
    order.id = nextOrderId_++;
    order.user = event.tokens[0];
    order.side = parseOrderSide(event.tokens[1]);
    order.driver = event.tokens[2];
    order.odds = std::stod(event.tokens[3]);
    order.quantity = std::stoi(event.tokens[4]);
    order.timestampMs = event.timestampMs;

    requireBook(order.driver);

    out_ << '[' << event.timestampMs << "] Order added: " << order.user << ' '
         << toString(order.side) << ' ' << order.driver << " @ " << formatDouble(order.odds)
         << " x " << order.quantity << '\n';

    matchOrder(std::move(order));
}

void Engine::handleFinishEvent(const Event& event) {
    raceState_.finishOrder = event.tokens;

    for (std::size_t index = 0; index < raceState_.finishOrder.size(); ++index) {
        DriverBook& book = requireBook(raceState_.finishOrder[index]);
        if (!book.active) {
            continue;
        }

        if (index == 0) {
            book.score += 15;
        } else if (index == 1) {
            book.score += 8;
        } else if (index == 2) {
            book.score += 4;
        }
    }

    out_ << '[' << event.timestampMs << "] FINISH:";
    for (const std::string& driver : raceState_.finishOrder) {
        out_ << ' ' << driver;
    }
    out_ << '\n';
    repricing("finish classification", event.timestampMs);
}

void Engine::repricing(const std::string& reason, long long timestampMs) {
    // Keep repricing deterministic so replay output stays stable run to run.
    applyPositionBonuses();

    out_ << '[' << timestampMs << "] Repriced WINNER market";
    if (!reason.empty()) {
        out_ << " (" << reason << ")";
    }
    out_ << '\n';

    for (const std::string& driver : raceState_.runningOrder) {
        DriverBook& book = requireBook(driver);
        if (!book.active) {
            continue;
        }

        book.currentOdds = scoreToOdds(book);
        out_ << "  " << driver << " -> " << formatDouble(book.currentOdds) << '\n';
    }

    for (const auto& [driver, book] : books_) {
        if (book.active) {
            continue;
        }
        out_ << "  " << driver << " -> CLOSED\n";
    }
}

void Engine::applyPositionBonuses() {
    for (auto& [driver, book] : books_) {
        if (!book.active) {
            continue;
        }

        int baseline = 70;
        const auto it = std::find(raceState_.runningOrder.begin(), raceState_.runningOrder.end(), driver);
        if (it == raceState_.runningOrder.end()) {
            book.score = std::max(book.score, baseline);
            continue;
        }

        const auto position = static_cast<int>(std::distance(raceState_.runningOrder.begin(), it));
        int bonus = 0;
        if (position == 0) {
            bonus = 10;
        } else if (position == 1) {
            bonus = 5;
        } else if (position == 2) {
            bonus = 2;
        }

        book.score = std::max(book.score, baseline + bonus);
    }
}

void Engine::matchOrder(Order&& incoming) {
    DriverBook& book = requireBook(incoming.driver);

    if (incoming.side == OrderSide::BACK) {
        // Backers lift the cheapest available lays first.
        auto layIt = book.lays.begin();
        while (incoming.quantity > 0 && layIt != book.lays.end() && incoming.odds >= layIt->first) {
            auto& queue = layIt->second;
            while (incoming.quantity > 0 && !queue.empty()) {
                Order& resting = queue.front();
                const int matchedQty = std::min(incoming.quantity, resting.quantity);
                const double tradeOdds = layIt->first;

                incoming.quantity -= matchedQty;
                resting.quantity -= matchedQty;
                logTrade(incoming.timestampMs, incoming.driver, tradeOdds, matchedQty, incoming.user, resting.user);

                if (resting.quantity == 0) {
                    queue.pop_front();
                }
            }

            if (queue.empty()) {
                layIt = book.lays.erase(layIt);
            } else {
                ++layIt;
            }
        }
    } else {
        // Layers trade against the most generous back prices first.
        auto backIt = book.backs.begin();
        while (incoming.quantity > 0 && backIt != book.backs.end() && backIt->first >= incoming.odds) {
            auto& queue = backIt->second;
            while (incoming.quantity > 0 && !queue.empty()) {
                Order& resting = queue.front();
                const int matchedQty = std::min(incoming.quantity, resting.quantity);
                const double tradeOdds = backIt->first;

                incoming.quantity -= matchedQty;
                resting.quantity -= matchedQty;
                logTrade(incoming.timestampMs, incoming.driver, tradeOdds, matchedQty, resting.user, incoming.user);

                if (resting.quantity == 0) {
                    queue.pop_front();
                }
            }

            if (queue.empty()) {
                backIt = book.backs.erase(backIt);
            } else {
                ++backIt;
            }
        }
    }

    if (incoming.quantity > 0) {
        addRestingOrder(std::move(incoming));
    }
}

void Engine::addRestingOrder(Order&& order) {
    DriverBook& book = requireBook(order.driver);
    if (order.side == OrderSide::BACK) {
        book.backs[order.odds].emplace_back(std::move(order));
    } else {
        book.lays[order.odds].emplace_back(std::move(order));
    }
}

void Engine::logTrade(long long timestampMs, const std::string& driver, double tradeOdds, int quantity,
                      const std::string& backUser, const std::string& layUser) {
    metrics_.recordTrade();
    out_ << '[' << timestampMs << "] Trade matched: " << driver << " @ " << formatDouble(tradeOdds)
         << " x " << quantity << " (back=" << backUser << ", lay=" << layUser << ")\n";
}

double Engine::scoreToOdds(const DriverBook& book) const {
    if (!book.active) {
        return 999.0;
    }

    // Simple inverse scoring model: better race state means shorter odds.
    const double score = static_cast<double>(std::max(book.score, 1));
    const double odds = 150.0 / score;
    return std::clamp(odds, 1.20, 25.0);
}

int Engine::driverBaseScore(std::size_t index) const {
    static const int baselines[] = {100, 90, 85, 80, 75};
    return baselines[index < 5 ? index : 4];
}

DriverBook& Engine::requireBook(const std::string& driver) {
    const auto it = books_.find(driver);
    if (it == books_.end()) {
        throw std::runtime_error("Unknown driver: " + driver);
    }
    return it->second;
}

const DriverBook& Engine::requireBook(const std::string& driver) const {
    const auto it = books_.find(driver);
    if (it == books_.end()) {
        throw std::runtime_error("Unknown driver: " + driver);
    }
    return it->second;
}

OrderSide Engine::parseOrderSide(const std::string& sideToken) {
    if (sideToken == "BACK") {
        return OrderSide::BACK;
    }
    if (sideToken == "LAY") {
        return OrderSide::LAY;
    }
    throw std::runtime_error("Unknown order side: " + sideToken);
}

const char* Engine::toString(OrderSide side) {
    return side == OrderSide::BACK ? "BACK" : "LAY";
}
