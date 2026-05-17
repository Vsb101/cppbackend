#pragma once

#include <boost/json.hpp>
#include "../model/map.h"
#include "../model/dog.h"
#include "../model/game_session.h"
#include "../app/player.h"
#include "../infra/record.h" // Подключаем структуру Record

namespace model {

namespace json = boost::json;

// Сериализация персонажа
void tag_invoke(json::value_from_tag, json::value& jv, const Dog& dog);
void tag_invoke(json::value_from_tag, json::value& jv, const LostObject& loot);
void tag_invoke(json::value_from_tag, json::value& jv, const Dog::BagItem& item);
// ---------------------------

// Сериализация статики (В JSON)
void tag_invoke(json::value_from_tag, json::value& jv, const Road& road);
void tag_invoke(json::value_from_tag, json::value& jv, const Building& building);
void tag_invoke(json::value_from_tag, json::value& jv, const Office& office);
void tag_invoke(json::value_from_tag, json::value& jv, const Map& map);

// Десериализация статики (ИЗ JSON)
Road tag_invoke(json::value_to_tag<Road>, const json::value& jv);
Building tag_invoke(json::value_to_tag<Building>, const json::value& jv);
Office tag_invoke(json::value_to_tag<Office>, const json::value& jv);
Map tag_invoke(json::value_to_tag<Map>, const json::value& jv);

}  // namespace model

namespace app {

namespace json = boost::json;

void tag_invoke(json::value_from_tag, json::value& jv, const Player& player);

}  // namespace app

// Новое пространство имен для сериализации рекордов из базы данных
namespace infra {

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Record& record);

}  // namespace infra
