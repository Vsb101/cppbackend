#include "app.h"
#include "player.h"
#include <stdexcept>
#include <algorithm>
#include "../infra/state_listener.h"
#include "../other/sql_queries.h"

namespace app {

using namespace std::literals;

// Ленивая инициализация: схема создаётся при первом запросе
void Application::InitDatabase() {}

void Application::SaveRetiredDogRecord(const std::string& name, int score, double play_time) {
    try {
        auto pool = GetOrCreateDbPool();
        if (!pool) return;
        PoolConnectionHolder holder{*pool};
        pqxx::work tx{holder.Get()};
        
        tx.exec_params(
            db::queries::INSERT_RETIRE_RECORD,
            name, score, play_time
        );
        tx.commit();
    } catch (const std::exception&) {
        // Graceful degradation: рекорд теряется, но сервер не падает
    }
}

std::vector<RecordItem> Application::GetRecords(int start, int max_items) const {
    std::vector<RecordItem> records;
    try {
        auto pool = GetOrCreateDbPool();
        if (!pool) return records;
        PoolConnectionHolder holder{*pool};
        pqxx::read_transaction tx{holder.Get()};
        
        auto rows = tx.exec_params(
            db::queries::SELECT_RETIRE_RECORDS,
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
    } catch (const std::exception&) {
        // БД недоступна — возвращаем пустой список
    }
    return records;
}

std::pair<Token, util::Tagged<size_t, Player>> Application::JoinGame(
    const std::string& player_name, 
    const model::Map::Id& map_id) {
    
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto session = game_.FindOrCreateSession(map_id);
    if (!session) {
        throw std::invalid_argument("Map not found"s);
    }

    auto dog = session->CreateDog(player_name, randomize_spawn_);
    
    auto player = std::make_shared<Player>(util::Tagged<size_t, Player>{next_player_id_++}, player_name);
    
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
    
    // Любая команда сбрасывает таймер бездействия (включая "")
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
            
            // Callback вызывается под lock session->mutex_ (внутри Update).
            // SaveRetiredDogRecord → GetOrCreateDbPool → lock(mutex_).
            // Поэтому mutex_ — recursive, иначе deadlock.
            session->Update(dt_seconds, retirement_time_limit, [this](const std::shared_ptr<model::Dog>& retired_dog) {
                if (!retired_dog) return;

                SaveRetiredDogRecord(retired_dog->GetName(), retired_dog->GetScore(), retired_dog->GetPlayTime());

                auto dog_id = retired_dog->GetId();
                auto p_it = std::find_if(players_.begin(), players_.end(), [&dog_id](const auto& player) {
                    return player->GetDog() && player->GetDog()->GetId() == dog_id;
                });

                if (p_it != players_.end()) {
                    tokens_.RemovePlayer(*p_it);
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
