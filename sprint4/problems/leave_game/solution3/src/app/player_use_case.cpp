#include "player_use_case.h"

#include <string>
#include <set>
#include <stdexcept>

namespace app {

bool PlayerAction::IsDirection() const {
    // Список допустимых направлений
    std::set<std::string> valid_directions = {
        {static_cast<char>(model::Direction::NORTH)}, 
        {static_cast<char>(model::Direction::SOUTH)}, 
        {static_cast<char>(model::Direction::EAST)}, 
        {static_cast<char>(model::Direction::WEST)}
    };
    return valid_directions.find(move_) != valid_directions.end();
}

bool PlayerAction::IsStop() const {
    using namespace std::string_literals;
    return move_ == ""s;
}


PlayerActionUseCase::PlayerActionUseCase(PlayerTokens& player_tokens) 
    : player_tokens_{&player_tokens} {
}

// Изменяет направление движения пса
PlayerActionResult PlayerActionUseCase::ExecutePlayerAction(Token token, PlayerAction action) {
    if ( auto self_player = player_tokens_->FindPlayerByToken(token) ) {
        std::optional<model::Direction> move;
        double dog_speed;
        if ( action.IsStop() ) {
            dog_speed = 0.0;
        } else if ( action.IsDirection() ) {
            move = action.GetMoveAsDirection();
        } else {
            throw PlayerActionError{PlayerActionErrorReason::InvalidMove};
        }

        dog_speed = self_player->GetSession()->GetMap().GetDogSpeed().value();
        self_player->SetDogSpeed(dog_speed, move);
    } else {
        throw PlayerActionError{PlayerActionErrorReason::InvalidToken};
    }
   
    return PlayerActionResult{};
}

}  // namespace app
