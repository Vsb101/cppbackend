#pragma once

#include "players.h"

#include <optional>

namespace app {

// Класс реализует хранение допустимых действий игрока
// и преобразование их в различное представление
class PlayerAction {
public:
    PlayerAction(std::string move) 
        : move_(std::move(move)) {
        }

    std::string GetMoveAsString() const {
        return move_;
    }

    std::optional<model::Direction> GetMoveAsDirection() const {
        if (this->IsDirection()) {
            return model::Direction(move_[0]);
        }
        return std::nullopt;
    }

    bool IsDirection() const;
    bool IsStop() const;

private:
    std::string move_;
};

enum class PlayerActionErrorReason {
    InvalidToken,
    InvalidMove
};

struct PlayerActionError {
    PlayerActionErrorReason reason_;
};

class PlayerActionResult {
public:
    PlayerActionResult() {}

private:
};


class PlayerActionUseCase {
public:
    PlayerActionUseCase(PlayerTokens& player_tokens);
    // Подключает игрока с указанным именем (пса) к указанной карте
    PlayerActionResult ExecutePlayerAction(Token token, PlayerAction action);

private:
    PlayerTokens* player_tokens_;
};

}  // namespace app
