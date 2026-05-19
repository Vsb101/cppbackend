#include "players.h"
#include "game_session.h"

#include <boost/smart_ptr/make_shared_object.hpp>
#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>

namespace app {
using namespace std::literals;

///  ---  PlayerTokens  ---  ///

Player* PlayerTokens::FindPlayerByToken(Token token) {
    if ( auto it = token_to_player.find(token); it != token_to_player.end() ) {
        return token_to_player[token];
    }
    return nullptr;
}

Token PlayerTokens::AddPlayer(Player& player) {
    Token token(GenerateToken());
    // @todo: Обработка ошибок. Вдруг токены закончились и попался повторяющийся
    token_to_player[token] = &player;
    return token;
}
///  ---  PlayerTokens  ---  ///

///  ---  Players  ---  ///

Player& Players::Add(model::Dog* dog, model::GameSession& session) {
    auto index = players_.size();
    Player::Id id(index);

    try {
        // Добавляем игрока
        players_.emplace_back(std::make_shared<Player>(id, *dog, session));
        return *players_.back();
    } catch (...) {
        throw std::bad_alloc{};
    }
}

void Players::Add(std::unique_ptr<Player> player) {
    auto name = model::Dog::Name(*player->GetName());
    auto index = *player->GetId(); 

    if ( index >= players_.size() ) {
        players_.resize(index + 1);
    }
    players_[index] =std::move(player);
}

Player* Players::FinByDog(const model::Dog& dog, const model::GameSession& session) {
    for (const auto& player : players_) {
        if ( &player->GetDog() == &dog && player->GetSession() == &session ) {
            return player.get();
        }
    }

    return nullptr;
}

const Players::PlayersContainer& Players::GetPlayers() const {
    return players_;
}

void Players::RemovePlayer(const Player::Id& player_id) {
    // Находим игрока
    for (int i = 0; i < players_.size(); i++) {
        auto player = players_[i];
        if ( player->GetId() == player_id ) {
            // Сначала удаляем его собаку
            player->GetSession()->RemoveDog(player->GetDog().GetId());
            // Удаляем игрока
            players_[i] = players_.back();
            players_.pop_back();
        }
    }
}

///  ---  Players  ---  ///


}  // namespace app
