#include "app.h"
#include "player.h"
#include <stdexcept>
#include <algorithm>
#include "../infra/state_listener.h"

namespace app {

using namespace std::literals;

// ============================================================================
// Инициализация базы данных и вспомогательные методы персистентности
// ============================================================================

void Application::InitDatabase() {
    // Удалено — теперь инициализация происходит в GetOrCreateDbPool()
}

void Application::SaveRetiredDogRecord(const std::string& name, int score, double play_time) {
    try {
        auto pool = GetOrCreateDbPool();
        if (!pool) return;
        PoolConnectionHolder holder{*pool};
        pqxx::work tx{holder.Get()};
        
        tx.exec_params(
            "INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3);",
            name, score, play_time
        );
        
        tx.commit();
    } catch (const std::exception& e) {
        // Ошибка записи в БД не должна приводить к крашу игрового тика
    }
}

std::vector<RecordItem> Application::GetRecords(int start, int max_items) const {
    std::vector<RecordItem> records;
    try {
        auto pool = GetOrCreateDbPool();
        if (!pool) return records;
        PoolConnectionHolder holder{*pool};
        pqxx::read_transaction tx{holder.Get()};
        
        // Запрос использует оптимизированный индекс
        auto rows = tx.exec_params(
            "SELECT name, score, play_time FROM retired_players "
            "ORDER BY score DESC, play_time ASC, name ASC LIMIT $1 OFFSET $2;",
            max_items, start
        );
        
        records.reserve(rows.size());
        for (const auto& row : rows) {
            records.push_back({
                row["name"].as<std::string>(),
                row["score"].as<int>(),
                row["play_time"].as<double>()
            });
        }
    } catch (const std::exception& e) {
        // Возвращаем пустой вектор при ошибке БД
    }
    return records;
}

// ============================================================================
// Игровая логика
// ============================================================================

std::pair<Token, util::Tagged<size_t, Player>> Application::JoinGame(
    const std::string& player_name, 
    const model::Map::Id& map_id) {
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto session = game_.FindOrCreateSession(map_id);
    if (!session) {
        throw std::invalid_argument("Map not found"s);
    }

    auto dog = session->CreateDog(player_name, randomize_spawn_);
    
    using PlayerId = util::Tagged<size_t, Player>;
    auto player = std::make_shared<Player>(PlayerId{next_player_id_++}, player_name);
    
    player->SetGameSession(session);
    player->SetDog(dog);
    
    players_.push_back(player);
    return {tokens_.AddPlayer(player), player->GetId()};
}

std::vector<std::shared_ptr<Player>> Application::GetPlayersInSession(const Token& token) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto player = tokens_.FindPlayerByToken(token);
    if (!player) return {};

    std::vector<std::shared_ptr<Player>> result;
    result.reserve(players_.size());
    auto session = player->GetSession();
    if (!session) return {};
    
    auto session_ptr = session.get();
    for (const auto& p : players_) {
        if (p->GetSession().get() == session_ptr) {
            result.push_back(p);
        }
    }
    return result;
}

std::shared_ptr<Player> Application::FindPlayerByToken(const Token& token) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return tokens_.FindPlayerByToken(token);
}

void Application::MovePlayer(const Token& token, std::string_view move_cmd) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto player = tokens_.FindPlayerByToken(token);
    if (!player) return;

    auto dog = player->GetDog();
    auto session = player->GetSession();
    if (!dog || !session || !session->GetMap()) return;

    double speed_val = session->GetMap()->GetDogSpeed();
    if (speed_val <= 0.0) {
        speed_val = game_.GetDefaultDogSpeed();
    }
    
    // При любой явной команде движения сбрасываем счетчик бездействия пса
    dog->ResetIdleTime();

    if (move_cmd == "L"sv) { 
        dog->SetSpeed({-speed_val, 0.0}); 
        dog->SetDirection(model::Direction::WEST); 
    } else if (move_cmd == "R"sv) { 
        dog->SetSpeed({speed_val, 0.0}); 
        dog->SetDirection(model::Direction::EAST); 
    } else if (move_cmd == "U"sv) { 
        dog->SetSpeed({0.0, -speed_val}); 
        dog->SetDirection(model::Direction::NORTH); 
    } else if (move_cmd == "D"sv) { 
        dog->SetSpeed({0.0, speed_val}); 
        dog->SetDirection(model::Direction::SOUTH); 
    } else if (move_cmd == ""sv) { 
        dog->SetSpeed({0.0, 0.0}); 
    }
}

void Application::Tick(std::chrono::milliseconds delta) {
    double dt_seconds = std::chrono::duration<double>(delta).count();
    double retirement_time_limit = game_.GetDogRetirementTime();
    
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        for (auto& [id, session] : game_.GetSessions()) {
            if (!session) continue;
            
            // Задаем callback для обработки удаления собаки сессией
            session->Update(dt_seconds, retirement_time_limit, [this](const std::shared_ptr<model::Dog>& retired_dog) {
                if (!retired_dog) return;

                // 1. Сохраняем заслуги в PostgreSQL
                SaveRetiredDogRecord(retired_dog->GetName(), retired_dog->GetScore(), retired_dog->GetPlayTime());

                // 2. Находим соответствующего игрока и зачищаем следы его активности
                auto dog_id = retired_dog->GetId();
                auto p_it = std::find_if(players_.begin(), players_.end(), [&dog_id](const auto& player) {
                    return player->GetDog() && player->GetDog()->GetId() == dog_id;
                });

                if (p_it != players_.end()) {
                    // Аннулируем токен
                    tokens_.RemovePlayer(*p_it); // Убедитесь, что метод умеет удалять по shared_ptr/Token
                    // Удаляем из списка активных игроков приложения
                    players_.erase(p_it);
                }
            });
        }
    }
    
    NotifyTick(delta);
}

void Application::SetApplicationListener(std::unique_ptr<infra::StateListener> listener) {
    listener_ = std::move(listener);
}

void Application::NotifyTick(std::chrono::milliseconds delta) {
    if (listener_) {
        listener_->OnTick(delta);
    }
}

void Application::NotifyShutdown() {
    if (listener_) {
        listener_->OnShutdown();
    }
}

void Application::RestorePlayerToken(const Token& token, std::shared_ptr<Player> player) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    tokens_.RestorePlayer(token, player);
}

void Application::ClearAllPlayers() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    tokens_.ClearAllTokens();
    players_.clear();
}

} // namespace app
