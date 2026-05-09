#pragma once
#include <boost/json.hpp>
#include <unordered_map>
#include "../model/map.h"

namespace app {

class ExtraData {
public:
    void SaveLootTypes(const model::Map::Id& id, boost::json::array loot_types) {
        loot_types_storage_[id] = std::move(loot_types);
    }

    const boost::json::array& GetLootTypes(const model::Map::Id& id) const {
        static const boost::json::array empty;
        if (auto it = loot_types_storage_.find(id); it != loot_types_storage_.end()) {
            return it->second;
        }
        return empty;
    }

private:
    using MapIdHasher = util::TaggedHasher<model::Map::Id>;
    std::unordered_map<model::Map::Id, boost::json::array, MapIdHasher> loot_types_storage_;
};

} // namespace app