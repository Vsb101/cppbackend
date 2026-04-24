#include "app.h"
#include "player.h"
#include <stdexcept>

namespace app {

using namespace std::literals;

std::pair<Token, util::Tagged<size_t, Player>> Application::JoinGame(
    const std::string& player_name, 
    const model::Map::Id& map_id) {
    
    std::lock_guard lock(mutex_);
    
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

std::shared_ptr<Player> Application::FindPlayerByToken(const Token& token) const {
    std::lock_guard lock(mutex_);
    return tokens_.FindPlayerByToken(token);
}

std::vector<std::shared_ptr<Player>> Application::GetPlayersInSession(const Token& token) const {
    std::lock_guard lock(mutex_);
    auto player = tokens_.FindPlayerByToken(token);
    if (!player) return {};

    std::vector<std::shared_ptr<Player>> result;
    auto session = player->GetSession();
    if (!session) return {};
    
    auto session_id = session->GetId();
    for (const auto& p : players_) {
        auto p_session = p->GetSession();
        if (p_session && p_session->GetId() == session_id) {
            result.push_back(p);
        }
    }
    return result;
}

void Application::MovePlayer(const Token& token, std::string_view move_cmd) {
    std::lock_guard lock(mutex_);
    auto player = tokens_.FindPlayerByToken(token);
    if (!player) return;

    auto dog = player->GetDog();
    auto session = player->GetSession();
    if (!dog || !session) return;

    double speed_val = session->GetMap()->GetDogSpeed();
    if (speed_val == 0.0) speed_val = game_.GetDefaultDogSpeed();
    
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
    std::lock_guard lock(mutex_);
    double dt_seconds = delta.count() / 1000.0;
    for (auto& [id, session] : game_.GetSessions()) {
        session->Update(dt_seconds);
    }
}

} // namespace app
