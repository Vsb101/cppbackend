#include "app.h"
#include "player.h"
#include <boost/json.hpp>
#include <stdexcept>
#include "../infra/state_listener.h"
#include "../logger/logger.h"

namespace json = boost::json;

namespace app {

using namespace std::literals;

std::pair<Token, util::Tagged<size_t, Player>> Application::JoinGame(
    const std::string& player_name, 
    const model::Map::Id& map_id) {
    
    std::lock_guard lock(mutex_);
    
    auto session = game_.FindOrCreateSession(map_id);
    if (!session) {
        // Это исключение поймает блок catch в ApiHandler и вернет 400 вместо падения сервера
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

    // Берем скорость из карты, если она 0 — берем дефолтную из игры
    double speed_val = session->GetMap()->GetDogSpeed();
    if (speed_val <= 0.0) {
        speed_val = game_.GetDefaultDogSpeed();
    }
    
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
    
    std::vector<std::tuple<std::string, int, double>> db_records;
    {
        std::lock_guard lock(mutex_);
        for (auto& [id, session] : game_.GetSessions()) {
            if (!session) continue;
            try {
                session->Update(dt_seconds);
            } catch (const std::exception& e) {
                logger::LogError("session_update_failed"sv, e.what());
                continue;
            }

            std::vector<std::shared_ptr<model::Dog>> retired_dogs;
            for (const auto& dog : session->GetDogs()) {
                dog->UpdateTime(dt_seconds);
                auto speed = dog->GetSpeed();
                if (speed.vx == 0.0 && speed.vy == 0.0) {
                    dog->AddIdleTime(dt_seconds);
                } else {
                    dog->ResetIdleTime();
                }
                if (dog->IsRetired(dog_retirement_time_)) {
                    logger::LogInfo("dog_retired"sv, json::object{
                        {"name", dog->GetName()},
                        {"idle_time", dog->GetIdleTime()},
                        {"limit", dog_retirement_time_},
                        {"play_time", dog->GetPlayTime()}
                    });
                    retired_dogs.push_back(dog);
                }
            }

            for (const auto& dog : retired_dogs) {
                if (retired_player_db_) {
                    db_records.emplace_back(dog->GetName(), dog->GetScore(), dog->GetPlayTime());
                }
                session->RemoveDog(dog->GetId());
                auto it = std::find_if(players_.begin(), players_.end(),
                    [&dog](const auto& p) { return p->GetDog() == dog; });
                if (it != players_.end()) {
                    tokens_.RemovePlayer(*it);
                    players_.erase(it);
                }
            }
        }
    }

    // Записываем в БД вне блокировки, чтобы не блокировать Application
    if (retired_player_db_) {
        for (const auto& [name, score, play_time] : db_records) {
            try {
                retired_player_db_->Save(name, score, play_time);
            } catch (const std::exception& e) {
                logger::LogError("db_save_failed"sv, e.what());
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

std::vector<RetiredPlayer> Application::GetRecords(size_t offset, size_t limit) {
    if (!retired_player_db_) {
        return {};
    }
    return retired_player_db_->GetRecords(offset, limit);
}

} // namespace app
