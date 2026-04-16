#include "model.h"

namespace model {

void Map::AddOffice(Office office) {
    auto id = office.GetId();
    warehouse_id_to_index_.emplace(id, offices_.size());
    offices_.emplace_back(std::move(office));
}

void Game::AddMap(Map map) {
    auto id = map.GetId();
    maps_.emplace_back(std::move(map));
    map_id_to_index_.emplace(id, maps_.size() - 1);
}

}  // namespace model
