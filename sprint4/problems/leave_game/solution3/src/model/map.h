#pragma once

#include "road.h"
#include "buildings.h"

#include <optional>

namespace model {

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name, unsigned int n_loot_types = 1, int bag_capacity = 1) noexcept
        : id_{std::move(id)}
        , name_{std::move(name)}
        , dog_speed_{std::nullopt}
        , n_loot_types_{n_loot_types}
        , bag_capacity_{bag_capacity} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    std::optional<DynamicDimension> GetDogSpeed() const noexcept {
        return dog_speed_;
    }

    void SetDogSpeed(DynamicDimension dog_speed) noexcept {
        dog_speed_ = dog_speed;
    }

    int GetBagCapacity() const noexcept {
        return bag_capacity_;
    }

    void SetBagCapacity(int bag_capacity) {
        bag_capacity_ = bag_capacity;
    }

    unsigned int GetNLootTypes() const noexcept {
        return n_loot_types_;
    }

    void SetNLootTypes(unsigned int n_loot_types) {
        n_loot_types_ = n_loot_types;
    }

    void AddOffice(const Office& office);
    Position GetRandomPointOnMap() const;


private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    std::optional<DynamicDimension> dog_speed_;
    unsigned int n_loot_types_;
    int bag_capacity_;


    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

}  // namespace model
