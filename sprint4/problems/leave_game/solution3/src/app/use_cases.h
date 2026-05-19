#pragma once

#include "players.h"

#include <vector>

namespace app {

class UseCases {
public:
    virtual void AddPlayer(const app::PlayerStatInfo& player) = 0;
    virtual std::vector<app::PlayerStatInfo> GetRecords(size_t start, size_t limit) = 0;
protected:
    ~UseCases() = default;
};

}  // namespace app
