#pragma once

#include "app/player.h"
#include "app/player_tokens.h"
#include "model/model.h"
#include <string>

namespace app {

/**
 * @brief Сценарий использования: Вход в игру
 * 
 * Инкапсулирует логику входа игрока в игру.
 */
class JoinGameUseCase {
public:
    struct Result {
        bool success;
        Player::Id player_id;
        Player::Token token;
        std::string error_code;
        std::string message;
    };

    JoinGameUseCase(model::Game& game, PlayerTokens& tokens, std::vector<Player>& players);

    Result Execute(std::string user_name, std::string map_id);

private:
    model::Game& game_;
    PlayerTokens& tokens_;
    std::vector<Player>& players_;
};

}  // namespace app
