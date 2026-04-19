#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility> // Для std::pair

#include "../model/game.h"
#include "player.h"
#include "player_tokens.h"

namespace app {

class Application {
public:
    explicit Application(model::Game game)
        : game_(std::move(game)) {}

    // Явно указываем Token и Player::Id
    std::pair<Token, Player::Id> JoinGame(
        const std::string& player_name, 
        const model::Map::Id& map_id);

    std::vector<std::shared_ptr<Player>> GetPlayersInSession(const Token& token) const;

    const model::Game::Maps& ListMaps() const noexcept { return game_.GetMaps(); }
    std::shared_ptr<model::Map> FindMap(const model::Map::Id& id) const noexcept {
        return game_.FindMap(id);
    }

private:
    model::Game game_;
    PlayerTokens tokens_; // Теперь компилятор должен это видеть
    std::vector<std::shared_ptr<Player>> players_;
    size_t next_player_id_ = 0;
};

} // namespace app
