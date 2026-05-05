#include "../model/game.h"
#include <stdexcept>
#include <chrono>

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
    if (auto it = sessions_.find(id); it != sessions_.end()) {
        return it->second;
    }
    
    auto map = FindMap(id);
    if (!map) return nullptr;

    // Создаем генератор: переводим секунды из конфига в миллисекунды
    auto period_ms = std::chrono::milliseconds(
        static_cast<int64_t>(loot_generator_config_.period * 1000)
    );
    
    loot_gen::LootGenerator loot_gen(
        period_ms,
        loot_generator_config_.probability,
        []() { return loot_gen::s_distribution(loot_gen::s_rng); }
    );

    // Создаем новую сессию с генератором
    auto session = std::make_shared<GameSession>(map, std::move(loot_gen));
    sessions_[id] = session;
    return session;
}

}  // namespace model
