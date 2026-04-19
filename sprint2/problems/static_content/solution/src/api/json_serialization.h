#pragma once

#include <boost/json.hpp>
#include "../model/map.h"
#include "../app/player.h"
#include <fstream>

namespace model {

namespace json = boost::json;

// Сериализация (В JSON)
void tag_invoke(json::value_from_tag, json::value& jv, const Road& road);
void tag_invoke(json::value_from_tag, json::value& jv, const Building& building);
void tag_invoke(json::value_from_tag, json::value& jv, const Office& office);
void tag_invoke(json::value_from_tag, json::value& jv, const Map& map);

// Десериализация (ИЗ JSON)
Road tag_invoke(json::value_to_tag<Road>, const json::value& jv);
Building tag_invoke(json::value_to_tag<Building>, const json::value& jv);
Office tag_invoke(json::value_to_tag<Office>, const json::value& jv);
Map tag_invoke(json::value_to_tag<Map>, const json::value& jv);

}  // namespace model

namespace app {

namespace json = boost::json;

void tag_invoke(json::value_from_tag, json::value& jv, const Player& player);

}  // namespace app
