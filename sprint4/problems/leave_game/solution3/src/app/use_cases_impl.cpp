#include "use_cases_impl.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddPlayer(const app::PlayerStatInfo& player) {
    players_.Save({PlayerId::New(), player.name, player.score, player.play_time});
}

std::vector<PlayerStatInfo> UseCasesImpl::GetRecords(size_t start, size_t limit) {
    return players_.GetRecords(start, limit);
}

}  // namespace app
