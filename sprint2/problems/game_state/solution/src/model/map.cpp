#include "map.h"
#include <stdexcept>
#include <string>

namespace model {

using namespace std::literals;

void Map::AddOffice(Office office) {
    if (office_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate office with id "s + *office.GetId());
    }

    const size_t index = offices_.size();
    // Сначала обновляем индекс, потом добавляем в вектор для безопасности исключений
    office_id_to_index_[office.GetId()] = index;
    try {
        offices_.push_back(std::move(office));
    } catch (...) {
        office_id_to_index_.erase(office.GetId());
        throw;
    }
}

}  // namespace model
