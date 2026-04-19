#include <boost/json.hpp>
#include "json_loader.h"
#include "../api/json_serialization.h"
#include <fstream>
#include <string>

namespace json_loader {
namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path) {
    std::ifstream ifs(json_path);
    if (!ifs) throw std::runtime_error("Can't open " + json_path.string());

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    auto value = json::parse(content);
    auto& root_obj = value.as_object();
    
    model::Game game;

    // Читаем глобальную скорость по умолчанию (если нет — 1.0)
    double default_speed = 1.0;
    if (root_obj.contains("defaultDogSpeed")) {
        default_speed = root_obj.at("defaultDogSpeed").as_double();
    }
    game.SetDefaultDogSpeed(default_speed);

    // Читаем карты вручную, чтобы подхватить скорость каждой
    auto& maps_array = root_obj.at("maps").as_array();
    for (const auto& map_jv : maps_array) {
        auto& map_obj = map_jv.as_object();
        
        // Десериализуем базу карты (дороги, здания)
        auto map = json::value_to<model::Map>(map_jv);

        //  Устанавливаем скорость для этой конкретной карты
        if (map_obj.contains("dogSpeed")) {
            map.SetDogSpeed(map_obj.at("dogSpeed").as_double());
        } else {
            map.SetDogSpeed(default_speed);
        }

        game.AddMap(std::move(map));
    }

    return game;
}
} // namespace json_loader
