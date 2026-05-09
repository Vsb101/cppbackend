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
#include "../infra/state_listener.h"

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
    [[nodiscard]] std::vector<std::shared_ptr<Player>> GetPlayersInSession(const Token& token) const;
    [[nodiscard]] std::shared_ptr<Player> FindPlayerByToken(const Token& token) const;
    void MovePlayer(const Token& token, std::string_view move_cmd);
    void Tick(std::chrono::milliseconds delta);

    [[nodiscard]] const model::Game::Maps& ListMaps() const noexcept { return game_.GetMaps(); }
    [[nodiscard]] std::shared_ptr<model::Map> FindMap(const model::Map::Id& id) const noexcept { return game_.FindMap(id); }
    
    const ExtraData& GetExtraData() const noexcept { return extra_data_; }
    const model::Game& GetGame() const noexcept { return game_; }

    bool IsTickAuto() const noexcept { return is_tick_auto_; }
    bool IsRandomizeSpawn() const noexcept { return randomize_spawn_; }

    // Методы для поддержки слушателей событий (паттерн Наблюдатель)
    void SetApplicationListener(std::unique_ptr<infra::StateListener> listener);
    void NotifyTick(std::chrono::milliseconds delta);
    void NotifyShutdown();
    
    // Методы для сериализации состояния
    [[nodiscard]] const PlayerTokens& GetPlayerTokens() const { return tokens_; }
    [[nodiscard]] const std::vector<std::shared_ptr<Player>>& GetPlayers() const { return players_; }
    [[nodiscard]] size_t GetNextPlayerId() const { return next_player_id_; }
    void SetNextPlayerId(size_t id) { next_player_id_ = id; }
    
    // Восстановление состояния
    void RestorePlayerToken(const Token& token, std::shared_ptr<Player> player);

    // Методы для полной очистки и восстановления
    void ClearAllPlayers();

private:
    model::Game game_;
    ExtraData extra_data_; 
    PlayerTokens tokens_;
    std::vector<std::shared_ptr<Player>> players_;
    size_t next_player_id_ = 0;
    
    bool is_tick_auto_;
    bool randomize_spawn_;
    
    mutable std::mutex mutex_; 
    std::unique_ptr<infra::StateListener> listener_;
};

} // namespace app
