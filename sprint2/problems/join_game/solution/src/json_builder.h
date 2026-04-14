#pragma once

#include "model.h"
#include <string>

#include <boost/json.hpp>

namespace json_builder {

/**
 * @brief Строит JSON-представление карты.
 *
 * Включает: id, name, roads, buildings, offices.
 */
std::string BuildMap(const model::Map& map);

/**
 * @brief Строит JSON-список карт: [{"id": "...", "name": "..."}, ...].
 */
std::string BuildMapsList(const model::Game& game);

} // namespace json_builder