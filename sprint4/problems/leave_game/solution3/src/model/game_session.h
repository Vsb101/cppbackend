#pragma once

#include "map.h"
#include "dog.h"
#include "loot_generator.h"

#include <atomic>
#include <set>
#include <memory>

namespace model {

class GameSession {
public:
    using Dogs = std::vector<std::shared_ptr<Dog>>;
    using Items = std::vector<std::shared_ptr<Item>>;

    GameSession(const Map& map, loot_gen::LootGenerator* loot_generator) noexcept
        : map_{&map}
        , loot_generator_{loot_generator} {
    }

    Dog* AddDog(Position pos, const Dog::Name& name);
    void AddDog(const Dog& dog);

    const Dogs& GetDogs() const noexcept {
        return dogs_;
    }

    void AddItem(Position pos, Item::Type& type);
    void AddItem(std::shared_ptr<Item> item);

    const Items& GetItems() const noexcept {
        return items_;
    }

    const Map& GetMap() const noexcept {
        return *map_;
    }

    void Tick(TimeType dt) noexcept;

    Dog* FindDog(const Dog::Id& id) noexcept;
    void RemoveDog(const Dog::Id& id);

private:
    std::optional<Item::Id> GetItemIdByIndex(size_t index);
    std::optional<Dog::Id> GetDogIdByIndex(size_t index);
    void ClearCollectedItems(const std::set<Item::Id>& collected_items);
    Position MoveDog(Dog& dog, TimeType dt) noexcept;

private:
    using DogIdHasher = util::TaggedHasher<Dog::Id>;
    using DogIdToIndex = std::unordered_map<Dog::Id, size_t, DogIdHasher>;
    using ItemIdHasher = util::TaggedHasher<Item::Id>;
    using ItemIdToIndex = std::unordered_map<Item::Id, size_t, ItemIdHasher>;

    Dogs dogs_;
    Items items_;
    const Map* map_;
    DogIdToIndex dog_id_to_index_;
    std::atomic<size_t> next_dog_index_{0};
    ItemIdToIndex item_id_to_index_;
    std::atomic<size_t> next_item_index_{0};

    loot_gen::LootGenerator* loot_generator_;
};

}  // namespace model
