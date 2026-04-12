#include "game_model.h"

#include <sstream>
#include <iomanip>

namespace game_model {

PlayerTokens::PlayerTokens()
    : generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()}
    , generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()} {
}

AuthToken PlayerTokens::GenerateToken() {
    std::uniform_int_distribution<std::mt19937_64::result_type> dist;
    
    std::uint64_t part1 = dist(generator1_);
    std::uint64_t part2 = dist(generator2_);
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << part1 << std::setw(16) << part2;
    
    return AuthToken{oss.str()};
}

const Player* PlayerTokens::FindPlayerByToken(const AuthToken& token) const {
    auto it = token_to_player_.find(token);
    if (it != token_to_player_.end()) {
        return it->second;
    }
    return nullptr;
}

void PlayerTokens::AddToken(const AuthToken& token, const Player* player) {
    token_to_player_[token] = player;
}

GameSession::GameSession(const model::Map* map)
    : map_(map) {
}

void GameSession::AddPlayer(Player player) {
    players_.emplace_back(std::move(player));
}

const Player* GameSession::FindPlayer(PlayerId id) const {
    for (const auto& player : players_) {
        if (player.id == id) {
            return &player;
        }
    }
    return nullptr;
}

void GameSessions::AddSession(const model::Map* map) {
    sessions_.emplace(map, map);
}

GameSession* GameSessions::GetOrCreateSession(const model::Map* map) {
    auto it = sessions_.find(map);
    if (it == sessions_.end()) {
        it = sessions_.emplace(map, map).first;
    }
    return &it->second;
}

GameSession* GameSessions::FindSession(const model::Map* map) {
    auto it = sessions_.find(map);
    if (it != sessions_.end()) {
        return &it->second;
    }
    return nullptr;
}

const Player* GameSessions::FindPlayerByToken(const AuthToken& token) const {
    return tokens_.FindPlayerByToken(token);
}

void GameSessions::AddPlayerToSession(const model::Map* map, Player player) {
    auto* session = GetOrCreateSession(map);
    
    session->AddPlayer(std::move(player));
    
    // Добавляем токен в генератор после добавления игрока
    tokens_.AddToken(session->Back().token, &session->Back());
}

}  // namespace game_model
