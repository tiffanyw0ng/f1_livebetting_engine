# F1 Live Betting Exchange Engine

A small C++ exchange simulator that makes a Formula 1 race feel live. It replays a historical-style event feed, updates winner odds as the race evolves, matches back and lay orders, and measures how quickly the engine processes each event.

## What It Does

This MVP replays a single race from a text file. For each event it:

- updates race state
- reprices the `WINNER` market for a small set of drivers
- accepts `BACK` and `LAY` orders
- matches orders using exchange-style price-time priority
- reports trades, latency, and throughput

## Architecture

The flow is intentionally simple: `Replay` loads the feed into a `std::vector<Event>` and sorts it by timestamp, `Engine` walks those events in order, updates race state, reprices the market, and routes new bets into per-driver books, and `Metrics` records latency and throughput so the hot path is easy to reason about.

## Performance-Oriented C++ Choices

- Stored replay events in `std::vector` for cache-friendly sequential access during playback.
- Used `std::unordered_map` for direct driver lookup and `std::map` for sorted odds levels inside back/lay books.
- Reduced copies in the order path with move-aware insertion and `emplace_back`.
- Measured per-event latency and total throughput with `std::chrono::steady_clock`.
- Kept the hot path value-oriented and avoided unnecessary heap ownership.

## Event Grammar

Supported lines:

```txt
0 START VER NOR LEC PIA
1000 ODDS VER 2.10
3000 LAP VER 1 92.5
8000 OVERTAKE NOR VER
12000 PIT NOR
18000 DNF LEC
5000 BET user1 BACK VER 2.05 50
25000 FINISH VER NOR PIA
```

## Matching Rules

- Each driver has a back book sorted by highest odds first.
- Each driver has a lay book sorted by lowest odds first.
- A `BACK` matches if `incoming_back_odds >= best_lay_odds`.
- A `LAY` matches if `best_back_odds >= incoming_lay_odds`.
- Trades execute at the resting order price.
- Partial fills are supported and any remainder rests on the book.

## Repricing Model

The pricing model is intentionally simple and deterministic. It is not trying to be a sharp betting model; it is trying to create believable live movement that is easy to explain and easy to test.

- grid position and running order contribute baseline score
- strong laps add a small bump
- overtakes add to the passer and penalize the passed driver
- pit stops subtract score
- DNFs close the runner
- finish classification adds a final score bump

Higher score maps to lower odds using a simple inverse function. The goal is consistency, not predictive realism.

## Build

With CMake:

```bash
cd /Users/tiffany/Desktop/MDST/f1_exchange
cmake -S . -B build
cmake --build build
```

Direct compiler fallback:

```bash
cd /Users/tiffany/Desktop/MDST/f1_exchange
mkdir -p build
clang++ -std=c++17 -Wall -Wextra -Wpedantic -O2 \
  src/main.cpp src/Engine.cpp src/Replay.cpp src/Metrics.cpp \
  -o build/f1_exchange
```

## Run

```bash
./build/f1_exchange data/sample_race.txt
```

## Example Output

```txt
[5000] Order added: user1 BACK VER @ 2.05 x 50
[5100] Trade matched: VER @ 2.05 x 50
[8000] OVERTAKE: NOR over VER
[8000] Repriced WINNER market (overtake event)
--- Metrics ---
Events processed: 20
Trades matched: 5
Avg latency (ns): ...
Max latency (ns): ...
Throughput (events/sec): ...
```

## Why This Project Is Interesting

It is small enough to finish, but it still gives you real systems topics to talk through: event replay, state management, sorted books, price-time matching, cache-friendly container choices, and latency measurement. It also leaves a clean path for future work like threaded replay, queueing, and microbenchmarks.

## Interview Pitch

I built a C++ F1 betting exchange simulator that replays race events as a live feed, updates winner-market odds, and matches back/lay bets in real time. I designed it to make performance-oriented choices explicit, including vector versus list tradeoffs, ordered versus hash-based containers, move-aware insertion, and latency measurement.
