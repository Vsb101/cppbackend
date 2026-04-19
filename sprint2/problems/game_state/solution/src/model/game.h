#pragma once

#include "../model/map.h"
#include "../model/game_session.h"

#include <memory>
#include <vector>
#include <unordered_map>

namespace model {

class Game {
public:
    using Maps = std::vector<std::shared_ptr<Map>>;

    // Добавляет карту в игру
    void AddMap(Map map);

    // Возвращает список всех карт
    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    // Ищет карту по ID
    std::shared_ptr<Map> FindMap(const Map::Id& id) const noexcept;

    // Находит существующую сессию или создает новую для указанной карты
    // ВАЖНО: Модель сама должна решать, создавать сессию или нет
    std::shared_ptr<GameSession> GetSession(const Map::Id& id);

    std::shared_ptr<GameSession> FindOrCreateSession(const Map::Id& id);

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;

    std::vector<std::shared_ptr<Map>> maps_;
    std::unordered_map<Map::Id, size_t, MapIdHasher> map_id_to_index_;
    
    // Карта ID_Карты -> Сессия
    std::unordered_map<Map::Id, std::shared_ptr<GameSession>, MapIdHasher> sessions_;
};

}  // namespace model
