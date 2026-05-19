#include "tick_use_case.h"

namespace app {

TickUseCase::TickUseCase(model::Game& game) 
    : game_{&game} {
    }

// Выполняет один шаг по времени в игре
TickResult TickUseCase::ExecuteTick(Tick tick) {
    auto dt = tick.GetTimeDelta();
    
    for ( auto session : game_->GetSessions() ) {
        session->Tick(dt);
    }
   
    return TickResult{};
}


}  // namespace app
