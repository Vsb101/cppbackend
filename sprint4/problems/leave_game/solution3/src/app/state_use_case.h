#pragma once

#include "dog.h"
#include "players.h"

namespace app {

enum class GetStateErrorReason {
    InvalidToken
};

struct GetStateError {
    GetStateErrorReason reason_;
};

class StatePlayerInfo {
public:
    StatePlayerInfo(Player::Id id, model::Dog& dog) 
        : id_{id}
        , dog_{dog} {
    }

    std::string GetIdAsString() const {
        return std::to_string(*id_);
    }

    const model::Dog& GetDog() const {
        return dog_;
    }

private:
    Player::Id id_;
    model::Dog dog_;
};

class GetStateResult {
public:
    using Players = std::vector<StatePlayerInfo>;
    using Items = std::vector<model::Item>;

public:
    Players players_;
    Items items_;

};

class GetStateUseCase {
public:
    GetStateUseCase(PlayerTokens& player_tokens, Players& players)
    : player_tokens_{&player_tokens}
    , players_{&players} {}

    // Подключает игрока с указанным именем (пса) к указанной карте
    GetStateResult GetState(Token token);

private:
    PlayerTokens* player_tokens_;
    Players* players_;
};

}  // namespace app
