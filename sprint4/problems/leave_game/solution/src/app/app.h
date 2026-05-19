#pragma once
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <chrono>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <pqxx/pqxx> // Необходима библиотека libpqxx

#include "../model/game.h"
#include "player_tokens.h"
#include "player.h"
#include "extra_data.h"
#include "../infra/state_listener.h"

namespace app {

class Player;

// --- Потокобезопасный пул соединений для PostgreSQL ---
class ConnectionPool {
public:
    using ConnectionPtr = std::shared_ptr<pqxx::connection>;

    ConnectionPool(size_t capacity, const std::string& db_url) {
        for (size_t i = 0; i < capacity; ++i) {
            pool_.push(std::make_shared<pqxx::connection>(db_url));
        }
    }

    ConnectionPtr GetConnection() {
        std::unique_lock lock{mutex_};
        cv_.wait(lock, [this] { return !pool_.empty(); });
        auto conn = pool_.front();
        pool_.pop();
        return conn;
    }

    void ReturnConnection(ConnectionPtr conn) {
        std::lock_guard lock{mutex_};
        pool_.push(std::move(conn));
        cv_.notify_one();
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<ConnectionPtr> pool_;
};

// RAII-обертка для автоматического возврата соединения в пул
class PoolConnectionHolder {
public:
    PoolConnectionHolder(ConnectionPool& pool) : pool_(pool), conn_(pool.GetConnection()) {}
    ~PoolConnectionHolder() { pool_.ReturnConnection(conn_); }
    pqxx::connection& Get() { return *conn_; }
private:
    ConnectionPool& pool_;
    ConnectionPool::ConnectionPtr conn_;
};

// Структура ответа для Leaderboard
struct RecordItem {
    std::string name;
    int score = 0;
    double play_time = 0.0;
};

class Application {
public:
    explicit Application(model::Game game, ExtraData extra_data, bool is_tick_auto, 
                    bool randomize_spawn, const std::string& db_url = "")
    : game_(std::move(game))
    , extra_data_(std::move(extra_data))
    , is_tick_auto_(is_tick_auto)
    , randomize_spawn_(randomize_spawn)
    , db_url_(db_url.empty() ? "postgresql://localhost" : db_url)
    , db_pool_(nullptr) { 
        // Отложенная инициализация БД — создадим пул при первом запросе
    }

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

    // Метод получения рекордов из базы данных
    [[nodiscard]] std::vector<RecordItem> GetRecords(int start, int max_items) const;

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
    // Инициализация структуры таблиц БД
    void InitDatabase();
    
    // Внутренний метод сохранения рекорда ушедшего на пенсию пса
    void SaveRetiredDogRecord(const std::string& name, int score, double play_time);

    model::Game game_;
    ExtraData extra_data_; 
    PlayerTokens tokens_;
    std::vector<std::shared_ptr<Player>> players_;
    size_t next_player_id_ = 0;
    
    bool is_tick_auto_;
    bool randomize_spawn_;
    
    mutable std::shared_ptr<ConnectionPool> db_pool_; // Пул соединений с БД (ленивая инициализация)
    std::string db_url_;  // URL базы данных для ленивой инициализации

    mutable std::mutex mutex_; 
    std::unique_ptr<infra::StateListener> listener_;
    
    // Ленивая инициализация пула соединений
    std::shared_ptr<ConnectionPool> GetOrCreateDbPool() const {
        std::lock_guard lock(mutex_);
        if (!db_pool_) {
            db_pool_ = std::make_shared<ConnectionPool>(4, db_url_);
            // Инициализируем схему БД при первом подключении
            try {
                PoolConnectionHolder holder{*db_pool_};
                pqxx::work tx{holder.Get()};
                tx.exec(R"(
                    CREATE TABLE IF NOT EXISTS retired_players (
                        id SERIAL PRIMARY KEY,
                        name VARCHAR(100) NOT NULL,
                        score INT NOT NULL,
                        play_time DOUBLE PRECISION NOT NULL
                    );
                )");
                tx.exec(R"(
                    CREATE INDEX IF NOT EXISTS idx_retired_players_score_time_name 
                    ON retired_players (score DESC, play_time ASC, name ASC);
                )");
                tx.commit();
            } catch (const std::exception& e) {
                // Логируем, но не бросаем — игра будет работать без БД
            }
        }
        return db_pool_;
    }
};

} // namespace app
