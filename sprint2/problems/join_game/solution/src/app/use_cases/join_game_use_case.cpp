#include "join_game_use_case.h"
#include <algorithm>

namespace app {

JoinGameUseCase::JoinGameUseCase(
    model::Game& game,
    PlayerTokens& tokens,
    std::vector<Player>& players)
    : game_(game)
    , tokens_(tokens)
    , players_(players) {
}

JoinGameUseCase::Result JoinGameUseCase::Execute(std::string user_name, std::string map_id) {
    // Проверка имени
    if (user_name.empty() || user_name.size() > 12) {
        return {false, {}, {}, "invalidArgument", "Invalid user name"};
    }

    // Поиск карты
    const model::Map* map = game_.FindMap(model::Map::Id{map_id});
    if (!map) {
        return {false, {}, {}, "mapNotFound", "Map not found"};
    }

    // Создание пса
    size_t dog_index = game_.CreateDog(std::move(user_name), map);

    // Создание игрока
    auto token = tokens_.GenerateToken();
    Player::Id player_id{static_cast<int>(players_.size())};
    players_.emplace_back(player_id, std::move(token), dog_index);
    const Player& player = players_.back();

    return {true, player.GetId(), player.GetToken(), "", ""};
}

}  // namespace app
