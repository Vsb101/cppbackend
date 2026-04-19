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

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    std::shared_ptr<Map> FindMap(const Map::Id& id) const noexcept;

    std::shared_ptr<GameSession> FindOrCreateSession(const Map::Id& id);

    void SetDefaultDogSpeed(double speed) { default_dog_speed_ = speed; }

    double GetDefaultDogSpeed() const noexcept { return default_dog_speed_; }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;

    std::vector<std::shared_ptr<Map>> maps_;
    std::unordered_map<Map::Id, size_t, MapIdHasher> map_id_to_index_;
    
    std::unordered_map<Map::Id, std::shared_ptr<GameSession>, MapIdHasher> sessions_;

    // Скорость по умолчанию
    double default_dog_speed_{1.0};
};

}  // namespace model
