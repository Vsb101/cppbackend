#pragma once

#include "game.h"
#include "players.h"
#include "app_serialization.h"

namespace app {

enum class AddPlayerErrorReason {
    InvalidName,
    InvalidMap
};

struct AddPlayerError {
    AddPlayerErrorReason reason_;
};

class AddPlayerResult {
public:
    AddPlayerResult(Token token, Player::Id player_id) 
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


class AddPlayerUseCase {
public:
    AddPlayerUseCase(model::Game& game, PlayerTokens& player_tokens, Players& players);
    // Добавляет ранеее сохранённого игрока к указанной карте
    AddPlayerResult AddPlayer(const serialization::PlayerRepr& player, const Token& token);

private:
    PlayerTokens* player_tokens_;
    Players* players_;
    model::Game* game_;
};

}  // namespace app
