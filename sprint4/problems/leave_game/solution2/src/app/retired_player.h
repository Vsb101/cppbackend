#pragma once

#include <string>

namespace app {

struct RetiredPlayer {
    std::string name;
    int score = 0;
    double play_time = 0.0;  // seconds
};

} // namespace app
