#pragma once

#include "map.h"

#include <boost/json/array.hpp>
#include <unordered_map>

namespace extra_data {

class ExtendedMap {
public:
    using LootTypes = boost::json::array;
    ExtendedMap(model::Map map, LootTypes loot_types)
        : map_{std::move(map)}
        , loot_types_(std::move(loot_types)) {
    }

    const model::Map& GetMap() const {
        return map_;
    }

    model::Map* GetMapPointer() {
        return &map_;
    }

    const LootTypes& GetLootTypes() const {
        return loot_types_;
    }

private:
    model::Map map_;
    LootTypes loot_types_;
};

using MapIdHasher = util::TaggedHasher<model::Map::Id>;
using MapsLootTypes = std::unordered_map<model::Map::Id, ExtendedMap::LootTypes, MapIdHasher>;

}  // namespace extra_data
