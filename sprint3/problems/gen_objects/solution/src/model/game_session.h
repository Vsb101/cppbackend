#pragma once

#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <map>
#include <chrono>
#include <mutex> // Добавили для потокобезопасности

#include "../other/tagged.h"
#include "dog.h"
#include "map.h"
#include "loot_generator.h"

namespace model {

struct LostObject {
    using Id = uint32_t;
    Id id;
    size_t type;
    std::pair<double, double> pos;
};

class GameSession {
 public:
    using Id = util::Tagged<std::string, GameSession>;

    GameSession(std::shared_ptr<Map> map, loot_gen::LootGenerator loot_gen);

    std::shared_ptr<Dog> CreateDog(const std::string& name, bool randomize = false);

    [[nodiscard]] const Id& GetId() const noexcept;
    [[nodiscard]] std::shared_ptr<Map> GetMap() const noexcept;
    
    // В многопоточной среде эти геттеры опасны без внешней блокировки,
    // но пока оставим их для совместимости
    [[nodiscard]] const std::vector<std::shared_ptr<Dog>>& GetDogs() const noexcept;
    [[nodiscard]] const std::map<uint32_t, LostObject>& GetLostObjects() const noexcept;

    // Обновление состояния за время dt
    void Update(double dt_seconds);

 private:
    // --- Потокобезопасность ---
    // mutable позволяет захватывать мьютекс в константных методах
    mutable std::mutex mutex_; 
    // --------------------------

    std::shared_ptr<Map> map_;
    Id id_;
    std::vector<std::shared_ptr<Dog>> dogs_;
    size_t next_dog_id_ = 0;
    
    loot_gen::LootGenerator loot_generator_;
    std::map<uint32_t, LostObject> lost_objects_;
    uint32_t next_loot_id_ = 0;

    void GenerateLoot(std::chrono::milliseconds delta);
    
    const Road* FindRoadAndBounds(Position pos) const;
    std::vector<const Road*> FindRoadsAtPosition(Position pos) const;
    bool IsPositionOnRoad(Position pos, const Road& road) const;
    void PutDogInRndPosition(std::shared_ptr<Dog> dog);
};

}  // namespace model
