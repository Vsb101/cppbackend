#pragma once

#include "players.h"

namespace app {

enum class ListPlayersErrorReason {
    InvalidToken
};

struct ListPlayersError {
    ListPlayersErrorReason reason_;
};

class PlayerInfo {
public:
    PlayerInfo(Player::Id id, Player::Name name) 
        : id_{id}
        , name_{name} {
    }

    std::string GetIdAsString() const {
        return std::to_string(*id_);
    }
    std::string GetNameAsString() const {
        return *name_;
    }

private:
    Player::Id id_;
    Player::Name name_;
};

using ListPlayersResult = std::vector<PlayerInfo>;

class ListPlayersUseCase {
public:
    ListPlayersUseCase(PlayerTokens& player_tokens, Players& players)
    : player_tokens_{&player_tokens}
    , players_{&players} {}

    // Полчает список игроков, "видимых" для игрока с указанным токеном
    ListPlayersResult GetPlayers(Token token);

private:
    PlayerTokens* player_tokens_;
    Players* players_;
};

}  // namespace app
