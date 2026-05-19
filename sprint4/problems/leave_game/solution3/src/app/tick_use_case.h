#pragma once

#include "game.h"

namespace app {

class Tick {
public:
    Tick(model::TimeType time_delta) 
        : time_delta_{time_delta} {
        }

    model::TimeType GetTimeDelta() const {
        return time_delta_;
    }

private:
    model::TimeType time_delta_;
};

enum class TickErrorReason {
    InvalidToken,
    InvalidMove
};

struct TickError {
    TickErrorReason reason_;
};

class TickResult {
public:
    TickResult() {}

private:
};


class TickUseCase {
public:
    TickUseCase(model::Game& game_);
    // Подключает игрока с указанным именем (пса) к указанной карте
    TickResult ExecuteTick(Tick tick);

private:
    model::Game* game_;
};

}  // namespace app
