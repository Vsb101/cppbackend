#pragma once

#include <filesystem>
#include <string>

#include <boost/json/fwd.hpp>

#include "model.h"

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path);
std::string ReadFile(const std::filesystem::path& path);
model::Map ParseMap(const boost::json::object& map_obj);
model::Road ParseRoad(const boost::json::object& road_obj);
model::Building ParseBuilding(const boost::json::object& building_obj);
model::Office ParseOffice(const boost::json::object& office_obj);

}  // namespace json_loader
