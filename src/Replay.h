#pragma once

#include "Event.h"

#include <string>
#include <vector>

class Replay {
public:
    static std::vector<Event> loadFromFile(const std::string& path);
};
