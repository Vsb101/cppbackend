#include "app.h"
#include "player.h"
#include <stdexcept>
#include <algorithm>
#include <tuple>
#include "../infra/state_listener.h"
#include "../infra/retirement_repository.h"
#include "../logger/logger.h"

namespace app {

std::vector<infra::Record> Application::GetRecords(size_t start, size_t max_items) const {
    // Операции чтения из БД защищаем мьютексом слоя приложения, если репозиторий используется параллельно
    std::lock_guard lock(mutex_);
    if (retirement_repo_) {
        return retirement_repo_->GetRecords(start, max_items);
    }
    return {};
}

std::pair<Token, util::Tagged<size_t, Player>> Application::JoinGame(
    const std::string& player_name, 
    const model::Map::Id& map_id) {
    
    std::lock_guard lock(mutex_);
    
    auto session = game_.FindOrCreateSession(map_id);
    if (!session) {
        throw std::invalid_argument("Map not found");
    }

    auto dog = session->CreateDog(player_name, randomize_spawn_);
    dog->SetJoinTime(current_game_time_);
    
    using PlayerId = util::Tagged<size_t, Player>;
    auto player = std::make_shared<Player>(PlayerId{next_player_id_++}, player_name);
    
    player->SetGameSession(session);
    player->SetDog(dog);
    
    players_.push_back(player);
    return {tokens_.AddPlayer(player), player->GetId()};
}

std::vector<std::shared_ptr<Player>> Application::GetPlayersInSession(const Token& token) const {
    std::lock_guard lock(mutex_);
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
    std::lock_guard lock(mutex_);
    return tokens_.FindPlayerByToken(token);
}

void Application::MovePlayer(const Token& token, std::string_view move_cmd) {
    std::lock_guard lock(mutex_);
    auto player = tokens_.FindPlayerByToken(token);
    if (!player) return;

    auto dog = player->GetDog();
    auto session = player->GetSession();
    if (!dog || !session || !session->GetMap()) return;

    double speed_val = session->GetMap()->GetDogSpeed();
    if (speed_val <= 0.0) {
        speed_val = game_.GetDefaultDogSpeed();
    }
    
    if (move_cmd == "L") { 
        dog->SetSpeed({-speed_val, 0.0}); 
        dog->SetDirection(model::Direction::WEST); 
        if (dog->IsInactive()) dog->StopInactivity();
    } else if (move_cmd == "R") { 
        dog->SetSpeed({speed_val, 0.0}); 
        dog->SetDirection(model::Direction::EAST); 
        if (dog->IsInactive()) dog->StopInactivity();
    } else if (move_cmd == "U") { 
        dog->SetSpeed({0.0, -speed_val}); 
        dog->SetDirection(model::Direction::NORTH); 
        if (dog->IsInactive()) dog->StopInactivity();
    } else if (move_cmd == "D") { 
        dog->SetSpeed({0.0, speed_val}); 
        dog->SetDirection(model::Direction::SOUTH); 
        if (dog->IsInactive()) dog->StopInactivity();
    } else if (move_cmd == "") { 
        dog->SetSpeed({0.0, 0.0}); 
        if (!dog->IsInactive()) {
            dog->StartInactivity(current_game_time_);
        }
    }
}

void Application::Tick(std::chrono::milliseconds delta) {
    double dt_seconds = std::chrono::duration<double>(delta).count();
    
    // Структуры для сбора данных, которые мы обработаем ПОСЛЕ выхода из-под мьютекса
    std::vector<std::tuple<std::string, int, double>> records_to_add;
    
    {
        std::lock_guard lock(mutex_);
        current_game_time_ += dt_seconds;
        
        // 1. Обновляем физику сессий
        for (const auto& [id, session] : game_.GetSessions()) {
            if (session) {
                session->Update(dt_seconds, current_game_time_);
            }
        }
        
        double retirement_time = game_.GetDogRetirementTime();
        std::vector<std::shared_ptr<Player>> players_to_remove;
        std::vector<Token> tokens_to_remove;
        
        // 2. Выявляем неактивных собак (учитываем также остановку об стену)
        for (auto& player : players_) {
            auto dog = player->GetDog();
            if (!dog) continue;
            
            if (dog->GetSpeed().vx == 0.0 && dog->GetSpeed().vy == 0.0) {
                if (!dog->IsInactive()) {
                    dog->StartInactivity(current_game_time_);
                }
            }
            
            if (dog->IsInactive()) {
                double inactive_duration = current_game_time_ - dog->GetInactivityStartTime();
                if (inactive_duration >= retirement_time) {
                    double play_time = current_game_time_ - dog->GetJoinTime();
                    
                    records_to_add.push_back({dog->GetName(), dog->GetScore(), play_time});
                    players_to_remove.push_back(player);
                    
                    if (auto token_opt = tokens_.FindTokenByPlayer(player)) {
                        tokens_to_remove.push_back(*token_opt);
                    }
                }
            }
        }

        
        // 3. Безопасное удаление игроков и токенов из памяти игры
        for (size_t i = 0; i < players_to_remove.size(); ++i) {
            auto player = players_to_remove[i];
            auto dog = player->GetDog();
            auto session = player->GetSession();
            
            if (dog && session) {
                session->RemoveDog(dog->GetId());
            }
            
            if (i < tokens_to_remove.size()) {
                tokens_.RemovePlayer(tokens_to_remove[i]);
            }
            
            players_.erase(
                std::remove(players_.begin(), players_.end(), player),
                players_.end()
            );
        }
    } // Здесь мьютекс уничтожается и освобождает игровой цикл

    // 4. Сохраняем рекорды в базу данных (ВНЕ МЬЮТЕКСА!)
    if (!records_to_add.empty() && retirement_repo_) {
        for (const auto& [name, score, play_time] : records_to_add) {
            logger::LogInfo("Adding record to DB synchronously", name);
            try {
                // Вызываем синхронный метод. Запрос выполнится до того, 
                // как метод Tick завершит работу и вернет ответ тестам!
                retirement_repo_->AddRecord(name, score, play_time);
            } catch (const std::exception& ex) {
                logger::LogError("Failed to save record", std::string(ex.what()));
            }
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
    std::lock_guard lock(mutex_);
    tokens_.RestorePlayer(token, player);
}

void Application::ClearAllPlayers() {
    std::lock_guard lock(mutex_);
    tokens_.ClearAllTokens();
    players_.clear();
}

} // namespace app
