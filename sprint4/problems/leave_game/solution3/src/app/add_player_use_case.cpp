#include "add_player_use_case.h"

#include <memory>
#include <stdexcept>

namespace app {

AddPlayerUseCase::AddPlayerUseCase(model::Game& game, PlayerTokens& player_tokens, Players& players) 
    : game_{&game}
    , player_tokens_{&player_tokens}
    , players_{&players} {
}

AddPlayerResult AddPlayerUseCase::AddPlayer(const serialization::PlayerRepr& player, const Token& token) {
    auto map = game_->FindMap(player.GetSession());
    if ( !map ) {
        throw AddPlayerError{AddPlayerErrorReason::InvalidMap};
    }
    
    if ( auto session = game_->FindSession(map->GetId()) ) {
        try {
            // Связываем игрока с собакой на карте
            auto dog = session->FindDog(player.GetDog());
            // Создаём нового игрока - тут достаточно информации
            auto pl = std::make_unique<Player>(player.GetId(), *dog, *session);
            // Связываем токен с игроком
            player_tokens_->AddPlayer(pl.get(), token);
            // Передаём объект в класс, который занимается храненим
            players_->Add(std::move(pl));
            return {token, player.GetId()};
        }
        catch ( std::invalid_argument err ) {
            throw AddPlayerError{AddPlayerErrorReason::InvalidName};
        }
    }
    throw AddPlayerError{AddPlayerErrorReason::InvalidMap};
}


}  // namespace app
