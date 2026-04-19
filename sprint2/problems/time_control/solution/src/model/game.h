#pragma once

#include "../model/map.h"
#include "../model/game_session.h"

#include <memory>
#include <vector>
#include <unordered_map>

namespace model {

class Game {
public:
    // Переносим наверх, чтобы использовать в объявлении GetSessions
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using Maps = std::vector<std::shared_ptr<Map>>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    std::shared_ptr<Map> FindMap(const Map::Id& id) const noexcept;

    std::shared_ptr<GameSession> FindOrCreateSession(const Map::Id& id);
    
    // Метод для получения всех сессий (нужен для Tick)
    const std::unordered_map<Map::Id, std::shared_ptr<GameSession>, MapIdHasher>& GetSessions() const noexcept {
        return sessions_;
    }

    // Методы для управления скоростью по умолчанию
    void SetDefaultDogSpeed(double speed) { default_dog_speed_ = speed; }

    double GetDefaultDogSpeed() const noexcept { return default_dog_speed_; }

private:
    std::vector<std::shared_ptr<Map>> maps_;
    std::unordered_map<Map::Id, size_t, MapIdHasher> map_id_to_index_;
    
    std::unordered_map<Map::Id, std::shared_ptr<GameSession>, MapIdHasher> sessions_;

    // По умолчанию 1.0 согласно ТЗ
    double default_dog_speed_ = 1.0;
};

}  // namespace model
