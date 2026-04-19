#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <chrono> // Обязательно для std::chrono

#include "../model/game.h"
#include "player.h"
#include "player_tokens.h"

namespace app {

class Application {
public:
    explicit Application(model::Game game)
        : game_(std::move(game)) {}

    // Вход в игру
    std::pair<Token, Player::Id> JoinGame(
        const std::string& player_name, 
        const model::Map::Id& map_id);

    // Получение списка игроков в сессии
    std::vector<std::shared_ptr<Player>> GetPlayersInSession(const Token& token) const;

    // Поиск игрока по токену (нужен для ApiHandler)
    std::shared_ptr<Player> FindPlayerByToken(const Token& token) const;

    // Управление персонажем
    void MovePlayer(const Token& token, std::string_view move_cmd);

    // Продвижение времени
    void Tick(std::chrono::milliseconds delta);

    // Работа с картами
    const model::Game::Maps& ListMaps() const noexcept { return game_.GetMaps(); }
    std::shared_ptr<model::Map> FindMap(const model::Map::Id& id) const noexcept {
        return game_.FindMap(id);
    }

private:
    model::Game game_;
    PlayerTokens tokens_;
    std::vector<std::shared_ptr<Player>> players_;
    size_t next_player_id_ = 0;
};

} // namespace app
