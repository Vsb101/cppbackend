#include "application.h"
#include <algorithm>

namespace app {

Application::Application(model::Game& game)
    : game_(game) {
}

Application::JoinGameResult Application::JoinGame(std::string user_name, std::string map_id) {
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

std::vector<Player> Application::GetPlayers(const Player::Token& token) const {
    // Пока возвращаем пустой список - будет реализовано позже
    return {};
}

const Player* Application::FindPlayerByToken(const Player::Token& token) noexcept {
    auto it = std::find_if(players_.begin(), players_.end(),
                          [&token](const Player& player) {
                              return player.GetToken() == token;
                          });
    return it != players_.end() ? &(*it) : nullptr;
}

}  // namespace app
