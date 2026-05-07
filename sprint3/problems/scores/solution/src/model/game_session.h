#pragma once

#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <chrono>
#include <mutex> 

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
    bool collected = false;
};

struct DogMovement {
    Position start_pos;
    Position end_pos;
    std::shared_ptr<Dog> dog;
};

class GameSession {
 public:
    using Id = util::Tagged<std::string, GameSession>;

    GameSession(std::shared_ptr<Map> map, loot_gen::LootGenerator loot_gen);

    [[nodiscard]] std::shared_ptr<Dog> CreateDog(const std::string& name, bool randomize = false);
    void Update(double dt_seconds);

    [[nodiscard]] const Id& GetId() const noexcept;
    [[nodiscard]] std::shared_ptr<Map> GetMap() const noexcept;
    [[nodiscard]] const std::vector<std::shared_ptr<Dog>>& GetDogs() const noexcept;
    [[nodiscard]] const std::unordered_map<uint32_t, LostObject>& GetLostObjects() const noexcept;

 private:
    // --- Движение ---
    [[nodiscard]] std::vector<DogMovement> PrepareDogMovements();
    void MoveDogs(double dt_seconds, std::vector<DogMovement>& movements);

    // --- Сбор предметов ---
    void ProcessGatherEvents(const std::vector<DogMovement>& movements);
    [[nodiscard]] Position GetEventPosition(const DogMovement& movement, double time) const;
    [[nodiscard]] bool IsDogAtOffice(const Position& pos) const;

    // --- Генерация лута ---
    void GenerateLoot(std::chrono::milliseconds delta);
    void PutDogInRndPosition(std::shared_ptr<Dog> dog);

    // --- Работа с дорогами ---
    [[nodiscard]] const Road* FindRoadAndBounds(Position pos) const;
    [[nodiscard]] std::vector<const Road*> FindRoadsAtPosition(Position pos) const;
    [[nodiscard]] bool IsPositionOnRoad(Position pos, const Road& road) const;

    // --- Данные ---
    std::shared_ptr<Map> map_;
    Id id_;
    std::vector<std::shared_ptr<Dog>> dogs_;
    size_t next_dog_id_ = 0;

    loot_gen::LootGenerator loot_generator_;
    std::unordered_map<uint32_t, LostObject> lost_objects_;
    uint32_t next_loot_id_ = 0;

    mutable std::mutex mutex_;
};

}  // namespace model
