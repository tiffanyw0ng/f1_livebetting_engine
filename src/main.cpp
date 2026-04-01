#include "Engine.h"
#include "Replay.h"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <replay_file>\n";
        return 1;
    }

    try {
        const std::string replayPath = argv[1];
        const auto events = Replay::loadFromFile(replayPath);

        Engine engine(std::cout);
        engine.process(events);
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
