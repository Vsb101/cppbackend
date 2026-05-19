#pragma once

#include "../model/map.h"
#include "../model/game_session.h"
#include "loot_generator.h"

#include <memory>
#include <vector>
#include <unordered_map>

namespace model {

struct LootGeneratorConfig {
    double period = 0.0;
    double probability = 0.0;
};

class Game {
public:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using Maps = std::vector<std::shared_ptr<Map>>;

    void AddMap(Map map);

    std::shared_ptr<Map> FindMap(const Map::Id& id) const noexcept;
    std::shared_ptr<GameSession> FindOrCreateSession(const Map::Id& id);
    
    const Maps& GetMaps() const noexcept { return maps_; }
    const std::unordered_map<Map::Id, std::shared_ptr<GameSession>, MapIdHasher>& GetSessions() const noexcept {
        return sessions_;
    }

    void SetDefaultDogSpeed(double speed) { default_dog_speed_ = speed; }
    double GetDefaultDogSpeed() const noexcept { return default_dog_speed_; }

    void SetLootGeneratorConfig(LootGeneratorConfig config) { loot_generator_config_ = config; }
    const LootGeneratorConfig& GetLootGeneratorConfig() const noexcept { return loot_generator_config_; }

    void SetDogRetirementTime(double time) noexcept { dog_retirement_time_ = time; }
    [[nodiscard]] double GetDogRetirementTime() const noexcept { return dog_retirement_time_; }

private:
    Maps maps_;
    std::unordered_map<Map::Id, size_t, MapIdHasher> map_id_to_index_;
    std::unordered_map<Map::Id, std::shared_ptr<GameSession>, MapIdHasher> sessions_;

    double default_dog_speed_{1.0};
    LootGeneratorConfig loot_generator_config_;
    double dog_retirement_time_{60};
};

}  // namespace model
