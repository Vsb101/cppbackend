#include "map.h"

namespace model {

void Map::AddOffice(const Office& office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

// Получение произвольной точки на дорогах карты
model::Position Map::GetRandomPointOnMap() const {
    // Выбираем дорогу
    size_t road_index = 0;
    const model::Road& road = roads_[road_index];

    // Выбираем точку на дороге
    model::Point start = road.GetStart();
    model::Point end = road.GetEnd();

    model::Position res;
    res.x = start.x;
    res.y = start.y;
    return res;
}

}  // namespace model
