#include "join_use_case.h"

#include <stdexcept>

namespace app {

// Проверка имени на правильность
bool isValidName(Player::Name name) {
    // На данный момент проверяем только пустоту строки
    return !name->empty();
}

JoinGameUseCase::JoinGameUseCase(model::Game& game, PlayerTokens& player_tokens, Players& players) 
    : game_{&game}
    , player_tokens_{&player_tokens}
    , players_{&players} {
}

// Подключает игрока с указанным именем (пса) к указанной карте
JoinGameResult JoinGameUseCase::JoinGame(model::Map::Id map_id, Player::Name name) {
    model::Dog::Name name_str(*name);
    if ( !isValidName(name) ) {
        throw JoinGameError{JoinGameErrorReason::InvalidName};
    }

    auto map = game_->FindMap(map_id);
    if ( !map ) {
        throw JoinGameError{JoinGameErrorReason::InvalidMap};
    }

    if ( auto session = game_->FindSession(model::Map::Id{map_id}) ) {
        auto spawn_point = map->GetRandomPointOnMap();
        try {
            auto dog = session->AddDog(spawn_point, std::move(name_str));
            auto& player = players_->Add(dog, *session);
            auto token = player_tokens_->AddPlayer(player);
            return {token, player.GetId()};
        }
        catch ( std::invalid_argument err ) {
            throw JoinGameError{JoinGameErrorReason::InvalidName};
        }
    }
    throw JoinGameError{JoinGameErrorReason::InvalidMap};
}


}  // namespace app
