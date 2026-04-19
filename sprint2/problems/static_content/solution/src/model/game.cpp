#include "../model/game.h"
#include <stdexcept>

namespace model {

using namespace std::literals;

void Game::AddMap(Map map) {
    auto id = map.GetId();
    if (map_id_to_index_.contains(id)) {
        throw std::invalid_argument("Map with id "s + *id + " already exists"s);
    }
    
    maps_.push_back(std::make_shared<Map>(std::move(map)));
    map_id_to_index_[id] = maps_.size() - 1;
}

std::shared_ptr<Map> Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return maps_.at(it->second);
    }
    return nullptr;
}

std::shared_ptr<GameSession> Game::FindOrCreateSession(const Map::Id& id) {
    // 1. Ищем в мапе sessions_
    if (auto it = sessions_.find(id); it != sessions_.end()) {
        return it->second;
    }
    
    // 2. Если нет, ищем карту
    auto map = FindMap(id);
    if (!map) return nullptr;

    // 3. Создаем новую сессию
    // ВАЖНО: Проверь конструктор GameSession (ниже)
    auto session = std::make_shared<GameSession>(map);
    sessions_[id] = session;
    return session;
}

}  // namespace model
