#pragma once
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <chrono>
#include <mutex>

#include "../model/game.h"
#include "player_tokens.h"
#include "player.h"
#include "extra_data.h"

namespace app {

class Player;

class Application {
public:
    explicit Application(model::Game game, ExtraData extra_data, bool is_tick_auto, bool randomize_spawn) 
        : game_(std::move(game))
        , extra_data_(std::move(extra_data))
        , is_tick_auto_(is_tick_auto)
        , randomize_spawn_(randomize_spawn) {}

    std::pair<Token, util::Tagged<size_t, Player>> JoinGame(const std::string& player_name, const model::Map::Id& map_id);
    std::vector<std::shared_ptr<Player>> GetPlayersInSession(const Token& token) const;
    std::shared_ptr<Player> FindPlayerByToken(const Token& token) const;
    void MovePlayer(const Token& token, std::string_view move_cmd);
    void Tick(std::chrono::milliseconds delta);

    const model::Game::Maps& ListMaps() const noexcept { return game_.GetMaps(); }
    std::shared_ptr<model::Map> FindMap(const model::Map::Id& id) const noexcept { return game_.FindMap(id); }
    
    const ExtraData& GetExtraData() const noexcept { return extra_data_; }
    const model::Game& GetGame() const noexcept { return game_; }

    bool IsTickAuto() const noexcept { return is_tick_auto_; }
    bool IsRandomizeSpawn() const noexcept { return randomize_spawn_; }

private:
    model::Game game_;
    ExtraData extra_data_; 
    PlayerTokens tokens_;
    std::vector<std::shared_ptr<Player>> players_;
    size_t next_player_id_ = 0;
    
    bool is_tick_auto_;
    bool randomize_spawn_;
    
    mutable std::mutex mutex_; 
};

} // namespace app
