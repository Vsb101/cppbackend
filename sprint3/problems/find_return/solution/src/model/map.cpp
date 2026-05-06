#include "map.h"
#include <stdexcept>
#include <string>

namespace model {

using namespace std::literals;

void Map::AddRoad(const Road& road) {
    roads_.push_back(road);
    const auto& last_road = roads_.back();
    if (last_road.IsHorizontal()) {
        horizontal_roads_[last_road.GetStart().y].push_back(&last_road);
    } else {
        vertical_roads_[last_road.GetStart().x].push_back(&last_road);
    }
}

void Map::AddOffice(Office office) {
    if (office_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate office with id "s + *office.GetId());
    }

    const size_t index = offices_.size();
    office_id_to_index_[office.GetId()] = index;
    try {
        offices_.push_back(std::move(office));
    } catch (...) {
        office_id_to_index_.erase(office.GetId());
        throw;
    }
}

const std::vector<const Road*>& Map::GetHorizontalRoadsAt(int y) const {
    static const std::vector<const Road*> empty;
    if (auto it = horizontal_roads_.find(y); it != horizontal_roads_.end()) {
        return it->second;
    }
    return empty;
}

const std::vector<const Road*>& Map::GetVerticalRoadsAt(int x) const {
    static const std::vector<const Road*> empty;
    if (auto it = vertical_roads_.find(x); it != vertical_roads_.end()) {
        return it->second;
    }
    return empty;
}

}  // namespace model
