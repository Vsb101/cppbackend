#include "players_use_case.h"

#include <stdexcept>

namespace app {

// Для игрока с указанным токеном выдаёт список игроков, которые находятся вместе с ним в одной сессии
ListPlayersResult ListPlayersUseCase::GetPlayers(Token token) {
    ListPlayersResult res;
    // Получаем игрока с заданным токеном
    if ( auto self_player = player_tokens_->FindPlayerByToken(token) ) {
        // Получаем сессию, к которой подключен игрок и список собак в сессии
        auto session = self_player->GetSession();
        auto dogs = session->GetDogs();;

        // Для каждой собаки находим игрока и складываем в результат
        for ( auto dog : dogs ) {
            auto dog_name = dog->GetName();
            auto player = players_->FinByDog(*dog, *session);
            res.push_back({player->GetId(), player->GetName()});
        }
    } else {
        throw ListPlayersError{ListPlayersErrorReason::InvalidToken};
    }

    return res;
}

}  // namespace app
