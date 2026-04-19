#include "app.h"
#include <stdexcept>

namespace app {

using namespace std::literals;

std::pair<Token, Player::Id> Application::JoinGame(
    const std::string& player_name, 
    const model::Map::Id& map_id
) {
    auto map = game_.FindMap(map_id);
    if (!map) {
        throw std::invalid_argument("Map not found"s);
    }

    auto session = game_.FindOrCreateSession(map_id);
    auto dog = session->CreateDog(player_name);

    Player::Id player_id{next_player_id_++};
    auto player = std::make_shared<Player>(player_id, player_name);
    player->SetGameSession(session);
    player->SetDog(dog);

    players_.push_back(player);
    Token token = tokens_.AddPlayer(player);

    return {token, player_id};
}

std::vector<std::shared_ptr<Player>> Application::GetPlayersInSession(const Token& token) const {
    auto requester = tokens_.FindPlayerByToken(token);
    if (!requester) {
        return {};
    }

    auto session = requester->GetSession();
    std::vector<std::shared_ptr<Player>> result;
    for (const auto& player : players_) {
        if (player->GetSession() == session) {
            result.push_back(player);
        }
    }
    return result;
}

std::shared_ptr<Player> Application::FindPlayerByToken(const Token& token) const {
    return tokens_.FindPlayerByToken(token);
}

// --- ДОБАВЛЕННЫЙ МЕТОД: Управление скоростью ---
void Application::MovePlayer(const Token& token, std::string_view move_cmd) {
    auto player = tokens_.FindPlayerByToken(token);
    if (!player) {
        throw std::invalid_argument("UnknownToken");
    }

    auto dog = player->GetDog();
    auto session = player->GetSession();
    if (!dog || !session) return;

    // Получаем скорость из настроек карты этой сессии
    double s = session->GetMap()->GetDogSpeed();

    if (move_cmd == "L") {
        dog->SetSpeed({-s, 0.0});
        dog->SetDirection(model::Direction::WEST);
    } else if (move_cmd == "R") {
        dog->SetSpeed({s, 0.0});
        dog->SetDirection(model::Direction::EAST);
    } else if (move_cmd == "U") {
        dog->SetSpeed({0.0, -s});
        dog->SetDirection(model::Direction::NORTH);
    } else if (move_cmd == "D") {
        dog->SetSpeed({0.0, s});
        dog->SetDirection(model::Direction::SOUTH);
    } else if (move_cmd == "") {
        dog->SetSpeed({0.0, 0.0});
    } else {
        throw std::invalid_argument("InvalidMove");
    }
}

void Application::Tick(std::chrono::milliseconds delta) {
    double seconds = delta.count() / 1000.0;
    for (auto& [id, session] : game_.GetSessions()) {
        session->Update(seconds);
    }
}

}  // namespace app
