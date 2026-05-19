#pragma once
#include "players.h"
#include "players_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(app::PlayerRepository& players)
        : players_{players} {
    }

    void AddPlayer(const app::PlayerStatInfo& player) override;
    std::vector<app::PlayerStatInfo> GetRecords(size_t start, size_t limit) override;

private:
    app::PlayerRepository& players_;
};

}  // namespace app
