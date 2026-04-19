#include "app.h"
#include <stdexcept>

namespace app {

using namespace std::literals;

// Обязательно указываем полный путь к типам, если возникнут споры
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

    // Используем tokens_, который теперь есть в приватных полях
    Token token = tokens_.AddPlayer(player);

    return {token, player_id};
}

std::vector<std::shared_ptr<Player>> Application::GetPlayersInSession(const Token& token) const {
    auto requester = tokens_.FindPlayerByToken(token);
    
    if (!requester) { // Теперь проверка (!) сработает
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


}  // namespace app
