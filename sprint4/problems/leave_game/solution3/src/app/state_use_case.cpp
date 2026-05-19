#include "state_use_case.h"

#include <stdexcept>

namespace app {

// Для игрока с указанным токеном выдаёт список игроков, которые находятся вместе с ним в одной сессии
GetStateResult GetStateUseCase::GetState(Token token) {
    GetStateResult res;
    // Получаем игрока с заданным токеном
    if ( auto self_player = player_tokens_->FindPlayerByToken(token) ) {
        // Получаем сессию, к которой подключен игрок и список собак в сессии
        auto session = self_player->GetSession();
        auto dogs = session->GetDogs();

        // Для каждой собаки находим игрока и складываем в результат
        for ( auto dog : dogs ) {
            auto dog_name = dog->GetName();
            auto player = players_->FinByDog(*dog, *session);
            StatePlayerInfo info{player->GetId(), *dog};
            res.players_.push_back(info);
        }

        // Для карты выдаём список лута на ней
        for ( auto item : session->GetItems() ) {
            res.items_.push_back(*item);
        }
    } else {
        throw GetStateError{GetStateErrorReason::InvalidToken};
    }

    return res;
}

}  // namespace app
