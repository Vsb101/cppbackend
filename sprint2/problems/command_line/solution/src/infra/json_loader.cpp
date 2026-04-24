#include <boost/json.hpp>
#include "json_loader.h"
#include "../api/json_serialization.h"
#include <fstream>
#include <string>

namespace json_loader {
namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path) {
    std::ifstream ifs(json_path);
    if (!ifs) {
        throw std::runtime_error("Can't open " + json_path.string());
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    
    boost::json::error_code ec;
    auto value = json::parse(content, ec);
    if (ec) {
        throw std::runtime_error("JSON parse error: " + ec.message());
    }
    
    if (!value.is_object()) {
        throw std::runtime_error("JSON root must be an object");
    }
    
    auto& root_obj = value.as_object();
    
    if (!root_obj.contains("maps")) {
        throw std::runtime_error("Missing required field 'maps' in JSON");
    }
    
    auto maps_it = root_obj.find("maps");
    if (!maps_it->value().is_array()) {
        throw std::runtime_error("Field 'maps' must be an array");
    }
    
    auto maps_array = maps_it->value().as_array();
    
    model::Game game;

    double default_speed = 1.0;
    if (root_obj.contains("defaultDogSpeed")) {
        default_speed = root_obj.at("defaultDogSpeed").as_double();
    }
    game.SetDefaultDogSpeed(default_speed);

    for (const auto& map_jv : maps_array) {
        auto& map_obj = map_jv.as_object();
        
        auto map = json::value_to<model::Map>(map_jv);

        if (map_obj.contains("dogSpeed")) {
            map.SetDogSpeed(map_obj.at("dogSpeed").as_double());
        } else {
            map.SetDogSpeed(default_speed);
        }

        game.AddMap(std::move(map));
    }

    return game;
}
}  // namespace json_loader
