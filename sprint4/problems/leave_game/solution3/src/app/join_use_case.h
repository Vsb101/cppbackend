#pragma once

#include "game.h"
#include "players.h"

namespace app {

enum class JoinGameErrorReason {
    InvalidName,
    InvalidMap
};

struct JoinGameError {
    JoinGameErrorReason reason_;
};

class JoinGameResult {
public:
    JoinGameResult(Token token, Player::Id player_id) 
        : token_{token}
        , player_id_{player_id} {
    }

    std::string GetTokenAsString() const {
        return *token_;
    }
    std::string GetPlayerIdAsString() const {
        return std::to_string(*player_id_);
    }

    auto GetPlayerId() const {
        return *player_id_;
    }

private:
    Token token_;
    Player::Id player_id_;
};


class JoinGameUseCase {
public:
    JoinGameUseCase(model::Game& game, PlayerTokens& player_tokens, Players& players);
    // Подключает игрока с указанным именем (пса) к указанной карте
    JoinGameResult JoinGame(model::Map::Id map_id, Player::Name name);

private:
    PlayerTokens* player_tokens_;
    Players* players_;
    model::Game* game_;
};

}  // namespace app
